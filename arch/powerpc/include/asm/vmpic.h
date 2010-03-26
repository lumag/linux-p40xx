/*
 * VMPIC private definitions and structure.
 *
 * Copyright (C) 2008-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#ifndef __VMPIC_H__
#define __VMPIC_H__

#include <linux/irq.h>

#define NR_INTERNAL_INTS 128
#define NR_EXTERNAL_INTS 16
#define NR_VMPIC_INTS (NR_EXTERNAL_INTS + NR_INTERNAL_INTS)

#define VMPIC_INFO(name) VMPIC_##name

#define VMPIC_VECPRI_POLARITY_NEGATIVE 0
#define VMPIC_VECPRI_POLARITY_POSITIVE 1
#define VMPIC_VECPRI_SENSE_EDGE 0
#define VMPIC_VECPRI_SENSE_LEVEL 0x2
#define VMPIC_VECPRI_POLARITY_MASK 0x1
#define VMPIC_VECPRI_SENSE_MASK 0x2

struct vmpic {
	/* The remapper for this VMPIC */
	struct irq_host	*irqhost;

	/* The "linux" controller struct */
	struct irq_chip	hc_irq;

	/* core int flag */
	int coreint_flag;
};

/*
 * Exported vmpic functions
 */

void __init vmpic_init(struct device_node *node, int coreint_flag);
unsigned int vmpic_get_irq(void);

#endif /* __VMPIC_H__ */
