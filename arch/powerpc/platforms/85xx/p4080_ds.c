/*
 * P4080 DS Setup
 *
 * Maintained by Kumar Gala (see MAINTAINERS for contact information)
 * Copyright 2009 Freescale Semiconductor Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/phy.h>
#include <linux/lmb.h>

#include <asm/system.h>
#include <asm/time.h>
#include <asm/machdep.h>
#include <asm/pci-bridge.h>
#include <mm/mmu_decl.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <asm/mpic.h>
#include <asm/swiotlb.h>

#include <linux/of_platform.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

void __init mpc85xx_ds_pic_init(void)
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
#endif

/*
 * Setup the architecture
 */
#ifdef CONFIG_SMP
void __init mpc85xx_smp_init(void);
#endif
static void __init mpc85xx_ds_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
	struct pci_controller *hose;
#endif
	dma_addr_t max = 0xffffffff;

	if (ppc_md.progress)
		ppc_md.progress("mpc85xx_ds_setup_arch()", 0);

#ifdef CONFIG_SMP
	mpc85xx_smp_init();
#endif

#ifdef CONFIG_PCI
	for_each_compatible_node(np, "pci", "fsl,p4080-pcie") {
		struct resource rsrc;
		of_address_to_resource(np, 0, &rsrc);
		if ((rsrc.start & 0xfffff) == primary_phb_addr)
			fsl_add_bridge(np, 1);
		else
			fsl_add_bridge(np, 0);

		hose = pci_find_hose_for_OF_device(np);
		max = min(max, hose->dma_window_base_cur +
				hose->dma_window_size);
	}
#endif

#ifdef CONFIG_SWIOTLB
	if (lmb_end_of_DRAM() > max) {
		ppc_swiotlb_enable = 1;
		set_pci_dma_ops(&swiotlb_dma_ops);
		ppc_md.pci_dma_dev_setup = pci_dma_dev_setup_swiotlb;
	}
#endif
	printk(KERN_INFO "P4080 DS board from Freescale Semiconductor\n");
}

int vsc824x_add_skew(struct phy_device *phydev);
#define PHY_ID_VSC8244                  0x000fc6c0
static int __init board_fixups(void)
{
	phy_register_fixup_for_uid(PHY_ID_VSC8244, 0xfffff, vsc824x_add_skew);

	return 0;
}
machine_device_initcall(p4080_ds, board_fixups);

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
	struct device_node *np;
	struct of_device *dev;
	int err;

	err = of_platform_bus_probe(NULL, of_device_ids, NULL);
	if (err)
		return err;

	/* Now probe the fake MDIO buses */
	for_each_compatible_node(np, NULL, "fsl,p4080ds-mdio") {
		dev = of_platform_device_create(np, NULL, NULL);
		if (!dev) {
			of_node_put(np);
			return -ENOMEM;
		}
	}

	for_each_compatible_node(np, NULL, "fsl,p4080ds-xmdio") {
		dev = of_platform_device_create(np, NULL, NULL);
		if (!dev) {
			of_node_put(np);
			return -ENOMEM;
		}
	}

	return 0;
}
machine_device_initcall(p4080_ds, declare_of_platform_devices);

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init p4080_ds_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	if (of_flat_dt_is_compatible(root, "fsl,P4080DS")) {
#ifdef CONFIG_PCI
		/* treat PCIe1 as primary,
		 * shouldn't matter as we have no ISA on the board
		 */
		primary_phb_addr = 0x0000;
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
void __init qman_init_early(void);
#endif
#ifdef CONFIG_FSL_BMAN_CONFIG
void __init bman_init_early(void);
#endif
#ifdef CONFIG_FSL_PME2_CTRL
void __init pme2_init_early(void);
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

define_machine(p4080_ds) {
	.name			= "P4080 DS",
	.probe			= p4080_ds_probe,
	.setup_arch		= mpc85xx_ds_setup_arch,
	.init_IRQ		= mpc85xx_ds_pic_init,
#ifdef CONFIG_PCI
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
#endif
	.get_irq		= mpic_get_coreint_irq,
	.restart		= fsl_rstcr_restart,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
	.init_early		= p4080_init_early,
};

machine_arch_initcall(p4080_ds, swiotlb_setup_bus_notifier);
