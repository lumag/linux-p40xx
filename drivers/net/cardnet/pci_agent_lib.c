/*
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <X.Xie@freescale.com>
 *
 * Description:
 * Freescale MPC85xx PCIE EP and PCI Agent basic lib
 *
 * Changelog:
 *
 * This file is part of the Linux Kernel
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/of_platform.h>

#include <asm/page.h>
#include <asm/byteorder.h>
#include <sysdev/fsl_soc.h>

#include "pci_agent_lib.h"

int setup_host_inbound_window(int pcie)
{
	volatile struct pcictrl85xx *pci;
	u32 value;
	u32 pci_addr;
	int order = 0;
	int winno = 0;

	winno = PCI_IBW_NUM - 1;

	if (winno < 0 || winno >= PCI_IBW_NUM) {
		printk(KERN_ERR "Window %d dose not exist\n", winno);
		return -1;
	}
	if (pcie) {
		pci = (struct pcictrl85xx *)ioremap(get_immrbase() +
						MPC85xx_PCIE_OFFSET,
						MPC85xx_PCIE_SIZE);
		pci_addr = PCIE_CCSR_BUS;
	} else {
		pci = (struct pcictrl85xx *)ioremap(get_immrbase() +
						MPC85xx_PCI1_OFFSET,
						MPC85xx_PCI1_SIZE);
		pci_addr = PCI_CCSR_BUS;
	}

	value = IMMRBAR_SIZE;
	while ((value = value>>1) > 1)
		order++;

	value = get_immrbase();
	pci->pci_ibw[winno].pitar = (value >> 12) & PITAR_TA_MASK;
	pci->pci_ibw[winno].piwbar = pci_addr >> 12;
	pci->pci_ibw[winno].piwar = PIWAR_EN | PIWAR_TRGT_MEM |
				PIWAR_RTT_SNOOP | PIWAR_WTT_SNOOP | order;

	iounmap((void __iomem *)pci);
	return 0;
}

int setup_agent_inbound_window(struct pci_agent_dev *dev)
{
	volatile struct pcictrl85xx *pci;
	u32 value;
	int order = 0;
	int winno = 0;

	winno = PCI_IBW_NUM - dev->window_num;

	if (winno < 0 || winno >= PCI_IBW_NUM) {
		printk(KERN_ERR "Window %d dose not exist\n", winno);
		return -1;
	}

	pci = (struct pcictrl85xx *)ioremap(get_immrbase() +
					MPC85xx_PCIE_OFFSET,
					MPC85xx_PCIE_SIZE);

	value = pci->pci_ibw[winno].piwar;
	if (value & PIWAR_EN) {
		value = dev->mem_size;
		while ((value = value>>1) > 1)
			order++;

		pci->pci_ibw[winno].pitar = (dev->local_addr>>12) &
							PITAR_TA_MASK;
		pci->pci_ibw[winno].piwar = PIWAR_EN | PIWAR_TRGT_MEM |
				PIWAR_RTT_SNOOP | PIWAR_WTT_SNOOP | order;

		iounmap((void __iomem *)pci);
		return 1;
	} else {
		iounmap((void __iomem *)pci);
		return -1;
	}
}

int setup_agent_outbound_window(int pcie, int winno)
{
	volatile struct pcictrl85xx *pci;
	u32 value;
	u32 pci_addr;
	u32 phy_addr;
	int order = 0;

	if (winno < 0 || winno > PCI_OBW_NUM) {
		printk(KERN_ERR "Window %d dose not exist\n", winno);
		return -1;
	}

	if (pcie) {
		pci = (struct pcictrl85xx *)ioremap(get_immrbase() +
						MPC85xx_PCIE_OFFSET,
						MPC85xx_PCIE_SIZE);
		phy_addr = PCIE_CCSR_BAR;
		pci_addr = PCIE_CCSR_BUS;
	} else {
		pci = (struct pcictrl85xx *)ioremap(get_immrbase() +
						MPC85xx_PCI1_OFFSET,
						MPC85xx_PCI1_SIZE);
		phy_addr = PCI_CCSR_BAR;
		pci_addr = PCI_CCSR_BUS;
	}

	value = IMMRBAR_SIZE;
	while ((value = value>>1) > 1)
		order++;

	pci->pci_obw[winno].potar = (pci_addr>>12) & POTAR_TA_MASK;
	pci->pci_obw[winno].powbar = (phy_addr>>12);
	pci->pci_obw[winno].powar = POWAR_EN | RTT_MEMORY_READ |
					WTT_MEMORY_WRITE | order;

	iounmap((void __iomem *)pci);
	return 0;
}

int ppc85xx_interrupt_init(void __iomem *messageu)
{
	volatile struct mesg_85xx *messager;
	struct device_node *np;

	messager = (struct mesg_85xx *)messageu;

	messager->mer = MU_MER_ENABLE;
	messager->msr = MU_MSR_CLR;

	np = of_find_compatible_node(NULL, NULL, "fsl,mpic-msg");
	if (np) {
		int irq;
		irq = irq_of_parse_and_map(np, 0);
		of_node_put(np);
		return irq;
	} else {
		printk(KERN_INFO "the device node isn't exist!\n");
		return 0;
	}
}

int ppc85xx_trigger_intr(u32 message, void __iomem *messageu)
{
	volatile struct mesg_85xx *messager;

	messager = (struct mesg_85xx *)messageu;

	out_be32(&(messager->msgr0), message);

	return 0;
}

int ppc85xx_readmsg(u32 *message, void __iomem *messageu)
{
	volatile struct mesg_85xx *messager;

	messager = (struct mesg_85xx *)messageu;

	*message = in_be32(&(messager->msgr0));
	return 0;
}

int ppc85xx_clean_interrupt(void __iomem *messageu)
{
	volatile struct mesg_85xx *messager;
	u32 status;

	messager = (struct mesg_85xx *)messageu;
	status = messager->msr;
	out_be32(&(messager->msr), (status & MU_MSR0_CLR));
	return 0;
}
