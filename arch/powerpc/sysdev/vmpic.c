/*
 *  arch/powerpc/kernel/vmpic.c
 *
 *  Driver for Virtual MPIC style interrupt controller for the
 *  MPC8578 multi-core platforms
 *
 *  Copyright (C) 2008-2009 Freescale Semiconductor, Inc. All rights reserved.
 *  Author: Ashish Kalra <ashish.kalra@freescale.com>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/bootmem.h>
#include <linux/spinlock.h>
#include <linux/of.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/smp.h>
#include <asm/machdep.h>
#include <asm/vmpic.h>
#include <asm/fsl_hcalls.h>

static struct vmpic *global_vmpic;
static DEFINE_SPINLOCK(vmpic_lock);

/*
 * Linux descriptor level callbacks
 */


void vmpic_unmask_irq(unsigned int irq)
{
	unsigned int src = virq_to_hw(irq);

	fh_vmpic_set_mask(src, 0);
}

void vmpic_mask_irq(unsigned int irq)
{
	unsigned int src = virq_to_hw(irq);

	fh_vmpic_set_mask(src, 1);
}

void vmpic_end_irq(unsigned int irq)
{
	unsigned int src = virq_to_hw(irq);

	fh_vmpic_eoi(src);
}

/* Convert a cpu mask from logical to physical cpu numbers. */
static inline u32 vmpic_physmask(u32 cpumask)
{
	int i;
	u32 mask = 0;

	for (i = 0; i < NR_CPUS; ++i, cpumask >>= 1)
		mask |= (cpumask & 1) << get_hard_smp_processor_id(i);
	return mask;
}

void vmpic_set_affinity(unsigned int irq, const struct cpumask *dest)
{
	unsigned int src = virq_to_hw(irq);
	unsigned int config, prio, cpu_dest;
	struct cpumask tmp;
	unsigned long flags;

	cpus_and(tmp, *dest, cpu_online_map);

	spin_lock_irqsave(&vmpic_lock, flags);
	fh_vmpic_get_int_config(src, &config, &prio, &cpu_dest);
	fh_vmpic_set_int_config(src,config, prio, vmpic_physmask(cpus_addr(tmp)[0]));
	spin_unlock_irqrestore(&vmpic_lock, flags);
}

static unsigned int vmpic_type_to_vecpri(unsigned int type)
{
	/* Now convert sense value */

	switch(type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		return VMPIC_INFO(VECPRI_SENSE_EDGE) |
		       VMPIC_INFO(VECPRI_POLARITY_POSITIVE);

	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_BOTH:
		return VMPIC_INFO(VECPRI_SENSE_EDGE) |
		       VMPIC_INFO(VECPRI_POLARITY_NEGATIVE);

	case IRQ_TYPE_LEVEL_HIGH:
		return VMPIC_INFO(VECPRI_SENSE_LEVEL) |
		       VMPIC_INFO(VECPRI_POLARITY_POSITIVE);

	case IRQ_TYPE_LEVEL_LOW:
	default:
		return VMPIC_INFO(VECPRI_SENSE_LEVEL) |
		       VMPIC_INFO(VECPRI_POLARITY_NEGATIVE);
	}
}

int vmpic_set_irq_type(unsigned int virq, unsigned int flow_type)
{
	unsigned int src = virq_to_hw(virq);
	struct irq_desc *desc = get_irq_desc(virq);
	unsigned int vecpri, vold, vnew, prio, cpu_dest;
	unsigned long flags;

	if (flow_type == IRQ_TYPE_NONE)
		flow_type = IRQ_TYPE_LEVEL_LOW;

	desc->status &= ~(IRQ_TYPE_SENSE_MASK | IRQ_LEVEL);
	desc->status |= flow_type & IRQ_TYPE_SENSE_MASK;
	if (flow_type & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW))
		desc->status |= IRQ_LEVEL;

	vecpri = vmpic_type_to_vecpri(flow_type);

	spin_lock_irqsave(&vmpic_lock, flags);
	fh_vmpic_get_int_config(src, &vold, &prio, &cpu_dest);
	vnew = vold & ~(VMPIC_INFO(VECPRI_POLARITY_MASK) |
			VMPIC_INFO(VECPRI_SENSE_MASK));
	vnew |= vecpri;

	/*
	 * TODO : Add specific interface call for platform to set
	 * individual interrupt priorities.
	 * platform currently using static/default priority for all ints
	 */

	prio = 8;

	fh_vmpic_set_int_config(src, vecpri, prio, cpu_dest);

	spin_unlock_irqrestore(&vmpic_lock, flags);
	return 0;
}

static struct irq_chip vmpic_irq_chip = {
	.mask		= vmpic_mask_irq,
	.unmask		= vmpic_unmask_irq,
	.eoi		= vmpic_end_irq,
	.set_type	= vmpic_set_irq_type,
};

/* Return an interrupt vector or NO_IRQ if no interrupt is pending. */
unsigned int vmpic_get_irq(void)
{
	int irq;

	BUG_ON(global_vmpic == NULL);

	if (global_vmpic->coreint_flag)
		irq = mfspr(SPRN_EPR); /* if core int mode */
	else
		fh_vmpic_iack(&irq); /* legacy mode */

	if (irq == 0xFFFF)    /* 0xFFFF --> no irq is pending */
		return NO_IRQ;

	/*
         * this will also setup revmap[] in the slow path for the first
         * time, next calls will always use fast path by indexing revmap
         */
	return irq_linear_revmap(global_vmpic->irqhost, irq);
}

static int vmpic_host_match(struct irq_host *h, struct device_node *node)
{
	/* Exact match, unless vmpic node is NULL */
	return h->of_node == NULL || h->of_node == node;
}

static int vmpic_host_map(struct irq_host *h, unsigned int virq,
			 irq_hw_number_t hw)
{
	struct vmpic *vmpic = h->host_data;
	struct irq_chip *chip;

	/* Default chip */
	chip = &vmpic->hc_irq;

	set_irq_chip_data(virq, vmpic);
	/*
	 * using handle_fasteoi_irq as our irq handler, this will
	 * only call the eoi callback and suitable for the MPIC
	 * controller which set ISR/IPR automatically and clear the
	 * highest priority active interrupt in ISR/IPR when we do
	 * a specific eoi
	 */
	set_irq_chip_and_handler(virq, chip, handle_fasteoi_irq);

	/* Set default irq type */
	set_irq_type(virq, IRQ_TYPE_NONE);

	return 0;
}

static int vmpic_host_xlate(struct irq_host *h, struct device_node *ct,
			   u32 *intspec, unsigned int intsize,
			   irq_hw_number_t *out_hwirq, unsigned int *out_flags)

{
	/*
	 * interrupt sense values coming from the guest device tree
	 * interrupt specifiers can have four possible sense and
	 * level encoding information and they need to
	 * be translated between firmware type & linux type.
	 */

	static unsigned char map_of_senses_to_linux_irqtype[4] = {
		IRQ_TYPE_EDGE_RISING,
		IRQ_TYPE_LEVEL_LOW,
		IRQ_TYPE_LEVEL_HIGH,
		IRQ_TYPE_EDGE_FALLING,
	};

	*out_hwirq = intspec[0];
	if (intsize > 1)
		*out_flags = map_of_senses_to_linux_irqtype[intspec[1]];
	else
		*out_flags = IRQ_TYPE_NONE;
	return 0;
}

static struct irq_host_ops vmpic_host_ops = {
	.match = vmpic_host_match,
	.map = vmpic_host_map,
	.xlate = vmpic_host_xlate,
};

/*
 * Exported functions
 */

void __init vmpic_init(struct device_node *node, int coreint_flag)
{
	struct vmpic *vmpic;

	vmpic = alloc_bootmem(sizeof(struct vmpic));
	if (vmpic == NULL)
		return;

	memset(vmpic, 0, sizeof(struct vmpic));

	vmpic->irqhost = irq_alloc_host(of_node_get(node), IRQ_HOST_MAP_LINEAR,
				       NR_VMPIC_INTS,
				       &vmpic_host_ops, 0);

	if (vmpic->irqhost == NULL) {
		of_node_put(node);
		return;
	}

	vmpic->irqhost->host_data = vmpic;
	vmpic->hc_irq = vmpic_irq_chip;
	vmpic->hc_irq.set_affinity = vmpic_set_affinity;
	vmpic->coreint_flag = coreint_flag;

	global_vmpic = vmpic;
	irq_set_default_host(global_vmpic->irqhost);
}
