/*
 * P4080 Hypervisor Platform
 *
 * This is the platform file for the Freescale P4080 running under the
 * Freescale Hypervisor.
 *
 * Maintained by Kumar Gala (see MAINTAINERS for contact information)
 * Copyright 2008-2009 Freescale Semiconductor Inc.
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
#include <asm/vmpic.h>
#include <asm/fsl_hcalls.h>

#include <linux/of_platform.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

#undef DEBUG

#ifdef DEBUG
#define DBG(fmt, args...) printk(KERN_ERR "%s: " fmt, __FUNCTION__, ## args)
#else
#define DBG(fmt, args...)
#endif

void __init p4080_hv_vmpic_init(void)
{
	struct device_node *np;
	int coreint_flag = 1;

	np = of_find_node_by_name(NULL, "hypervisor");
	if (np) {
		if (of_find_property(np, "fsl,hv-pic-legacy", NULL))
			coreint_flag = 0;
		of_node_put(np);
	}

	np = of_find_compatible_node(NULL, NULL, "fsl,hv-vmpic");

	if (np == NULL) {
		printk(KERN_ERR "Could not find vmpic node\n");
		return;
	}

	vmpic_init(np, coreint_flag);
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
static void __init p4080_hv_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
#endif

	if (ppc_md.progress)
		ppc_md.progress("p4080_hv_setup_arch()", 0);

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

	printk("P4080 HV platform from Freescale Semiconductor\n");
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
machine_device_initcall(p4080_hv, declare_of_platform_devices);

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init p4080_ds_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	if (of_flat_dt_is_compatible(root, "fsl,hv-platform-p4080")) {
#ifdef CONFIG_PCI
	/* xxx - galak */
		primary_phb_addr = 0x8000;
#endif
		return 1;
	} else {
		return 0;
	}
}

/* FIXME: move to common HV file */
static void fsl_hv_restart(char *cmd)
{
	printk("hv restart\n");
	fh_partition_restart(-1);
}

static void fsl_hv_halt(void)
{
	printk("hv exit\n");
	fh_partition_stop(-1);
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
#ifdef CONFIG_HVC_FREESCALE
	udbg_init_hvc_fsl();
#endif
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

define_machine(p4080_hv) {
	.name			= "P4080 HV",
	.probe			= p4080_ds_probe,
	.setup_arch		= p4080_hv_setup_arch,
	.init_IRQ		= p4080_hv_vmpic_init,
#ifdef CONFIG_PCI
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
#endif
	.get_irq		= vmpic_get_irq,
	.restart		= fsl_hv_restart,
	.power_off		= fsl_hv_halt,
	.halt			= fsl_hv_halt,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
	.init_early		= p4080_init_early,
	.idle_loop		= wait_idle,
};
