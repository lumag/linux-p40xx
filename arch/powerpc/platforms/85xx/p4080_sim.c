/*
 * P4080 SIM Setup
 *
 * Maintained by Kumar Gala (see MAINTAINERS for contact information)
 * Copyright 2007-2009 Freescale Semiconductor Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>

#include <asm/system.h>
#include <asm/time.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <mm/mmu_decl.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <asm/mpic.h>

#include <linux/of_platform.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(fmt, args...) printk(KERN_ERR "%s: " fmt, __FUNCTION__, ## args)
#else
#define DBG(fmt, args...)
#endif

void __init mpc85xx_sim_pic_init(void)
{
	struct mpic *mpic;
	struct resource r;
	struct device_node *np = NULL;

	np = of_find_node_by_type(np, "open-pic");

	if (np == NULL) {
		printk(KERN_ERR "Could not find open-pic node\n");
		return;
	}

	if (of_address_to_resource(np, 0, &r)) {
		printk(KERN_ERR "Failed to map mpic register space\n");
		of_node_put(np);
		return;
	}

	mpic = mpic_alloc(np, r.start, MPIC_PRIMARY | MPIC_ENABLE_COREINT |
			  MPIC_BIG_ENDIAN | MPIC_BROKEN_FRR_NIRQS |
			  MPIC_SINGLE_DEST_CPU,
			0, 256, " OpenPIC  ");
	BUG_ON(mpic == NULL);

	mpic_init(mpic);
}

#ifdef CONFIG_PCI
static int primary_phb_addr;
#endif	/* CONFIG_PCI */

/*
 * Setup the architecture
 */
#ifdef CONFIG_SMP
extern void __init mpc85xx_smp_init(void);
#endif
static void __init mpc85xx_sim_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
#endif

	if (ppc_md.progress)
		ppc_md.progress("mpc85xx_sim_setup_arch()", 0);

#ifdef CONFIG_SMP
        mpc85xx_smp_init();
#endif

#ifdef CONFIG_PCI
	for_each_node_by_type(np, "pci") {
		if (of_device_is_compatible(np, "fsl,p4080-pcie")) {
			struct resource rsrc;
			of_address_to_resource(np, 0, &rsrc);
			if ((rsrc.start & 0xfffff) == primary_phb_addr)
				fsl_add_bridge(np, 1);
			else
				fsl_add_bridge(np, 0);
		}
	}
#endif

	printk("P4080 SIM board from Freescale Semiconductor\n");
}

static const struct of_device_id of_device_ids[] __devinitconst = {
	{
		.compatible	= "simple-bus"
	},
	{
		.compatible	= "fsl,dpaa"
	},
	{
		.compatible	= "fsl,rapidio-delta",
	},
	{}
};

static int __init declare_of_platform_devices(void)
{
	return of_platform_bus_probe(NULL, of_device_ids, NULL);
}
machine_device_initcall(p4080_sim, declare_of_platform_devices);

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init p4080_sim_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	if (of_flat_dt_is_compatible(root, "fsl,P4080SIM")) {
#ifdef CONFIG_PCI
	/* xxx - galak */
		primary_phb_addr = 0x8000;
#endif
		return 1;
	} else {
		return 0;
	}
}

/* Early setup is required for large chunks of contiguous (and coarsely-aligned)
 * memory. The following shoe-horns Qman/Bman "init_early" calls into the
 * platform setup to let them parse their CCSR nodes early on. */
#ifdef CONFIG_FSL_QMAN_CONFIG
extern __init void qman_init_early(void);
#endif
#ifdef CONFIG_FSL_BMAN_CONFIG
extern __init void bman_init_early(void);
#endif
#ifdef CONFIG_FSL_PME2_CTRL
extern __init void pme2_init_early(void);
#endif

static __init void p4080_init_early(void)
{
#ifdef CONFIG_FSL_QMAN_CONFIG
	qman_init_early();
#endif
#ifdef CONFIG_FSL_BMAN_CONFIG
	bman_init_early();
#endif
#ifdef CONFIG_FSL_PME2_CTRL
	pme2_init_early();
#endif
}

define_machine(p4080_sim) {
	.name			= "P4080 SIM",
	.probe			= p4080_sim_probe,
	.setup_arch		= mpc85xx_sim_setup_arch,
	.init_IRQ		= mpc85xx_sim_pic_init,
#ifdef CONFIG_PCI
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
#endif
	.get_irq		= mpic_get_coreint_irq,
	.restart		= fsl_rstcr_restart,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
	.init_early		= p4080_init_early,
};
