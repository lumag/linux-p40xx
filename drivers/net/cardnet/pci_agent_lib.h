/*
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <X.Xie@freescale.com>
 *
 * Description:
 * Freescale mpc85xx pci/pcie control registers memory map.
 *
 * Changelog:
 * Jan. 2009 Roy Zang <tie-fei.zang@freescale.com>
 * - Add PCI Express define for P4080
 *
 * This file is part of the Linux Kernel
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef PCI_AGENT_LIB_H
#define PCI_AGENT_LIB_H

#define PCI_VENDOR_ID_FREESCALE	0x1957
#define PCI_VENDOR_ID_MOTOROLA 0x1057
#define	PCI_DEVICE_ID_MPC8568E	0x0020
#define	PCI_DEVICE_ID_P4080	0x0400

#define PPC85XX_NETDRV_NAME	"boardnet: PPC85xx PCI Agent Ethernet Driver"
#define DRV_VERSION		"1.1"

#define PFX PPC85XX_NETDRV_NAME ": "

#define	IMMRBAR_SIZE	0x01000000

#define	AGENT_MEM_BASE_ADDR	0x00
#define	AGENT_MEM_SIZE		0x00001000

#define	HOST_LOCAL_SPACE_ADDR	0x80000000
#define	PCI_SPACE_ADDRESS	0x80000000
#define PCIE_SPACE_ADDRESS	0xa0000000
#define PCI_EXPRESS_MEM		0x20000000

#define PCI_CCSR_BAR		0x9ff00000
#define PCI_CCSR_BUS		PCI_CCSR_BAR
/* for p4080 pcie1 */

#define PCIE_CCSR_BAR		0x80000000
#define PCIE_CCSR_BUS		(PCIE_CCSR_BAR + 0x10000000)

#define MPC85xx_PCI1_OFFSET	0x8C00
#define MPC85xx_PCI1_SIZE	0x200
/* for p4080 pcie 1 */
#define MPC85xx_PCIE_OFFSET	0x200c00
#define MPC85xx_PCIE_SIZE	0x200
#define MPC85xx_MSG_OFFSET	0x41400
#define MPC85xx_MSG_SIZE	0x200
#define MPC85xx_MSGVP_OFFSET	0x51600
#define MPC85xx_MSGVP_SIZE	0x600

#define PCI_IBW_NUM		3
#define PCI_OBW_NUM		5

/*
 * PCI Controller Control and Status Registers
 */
struct pci_ob_w {
	u32	potar;
	u32	potear;
	u32	powbar;
	u8	res0[4];
	u32	powar;
	u8	res1[12];
};

struct pci_ib_w {
	u32	pitar;
	u8	res0[4];
	u32	piwbar;
	u32	piwbear;
	u32	piwar;
	u8	res1[12];
};

struct pcictrl85xx {
	struct pci_ob_w	pci_obw[5];
	u8		res0[256];
	struct pci_ib_w	pci_ibw[3];
};

/* For outbound window */
#define POTAR_TA_MASK		0x000fffff
#define POWAR_EN		0x80000000
#define RTT_MEMORY_READ		0x00040000
#define WTT_MEMORY_WRITE	0x00004000

/* For inbound window */
#define PITAR_TA_MASK		0x000fffff
#define PIBAR_MASK		0xffffffff
#define PIEBAR_EBA_MASK		0x000fffff
#define PIWAR_EN		0x80000000
#define PIWAR_PF		0x20000000
#define PIWAR_TRGT_MEM		0x00f00000
#define PIWAR_RTT_MASK		0x000f0000
#define PIWAR_RTT_NO_SNOOP	0x00040000
#define PIWAR_RTT_SNOOP		0x00050000
#define PIWAR_WTT_MASK		0x0000f000
#define PIWAR_WTT_NO_SNOOP	0x00004000
#define PIWAR_WTT_SNOOP		0x00005000
#define PIWAR_IWS_MASK		0x0000003F
#define PIWAR_IWS_4K		0x0000000B
#define PIWAR_IWS_8K		0x0000000C
#define PIWAR_IWS_16K		0x0000000D
#define PIWAR_IWS_32K		0x0000000E
#define PIWAR_IWS_64K		0x0000000F
#define PIWAR_IWS_128K		0x00000010
#define PIWAR_IWS_256K		0x00000011
#define PIWAR_IWS_512K		0x00000012
#define PIWAR_IWS_1M		0x00000013
#define PIWAR_IWS_2M		0x00000014
#define PIWAR_IWS_4M		0x00000015
#define PIWAR_IWS_8M		0x00000016
#define PIWAR_IWS_16M		0x00000017
#define PIWAR_IWS_32M		0x00000018
#define PIWAR_IWS_64M		0x00000019
#define PIWAR_IWS_128M		0x0000001A
#define PIWAR_IWS_256M		0x0000001B
#define PIWAR_IWS_512M		0x0000001C
#define PIWAR_IWS_1G		0x0000001D
#define PIWAR_IWS_2G		0x0000001E

struct mesg_85xx {
	u32	msgr0;
	u8	res0[12];
	u32	msgr1;
	u8	res1[12];
	u32	msgr2;
	u8	res2[12];
	u32	msgr3;
	u8	res3[204];
	u32	mer;
	u8	res4[12];
	u32	msr;
	u8	res5[236];
};

#define MU_MER_ENABLE	0x0000000F
#define MU_MSR_CLR	0x0000000F
#define MU_MSR0_CLR	0x00000001
#define MU_MSR1_CLR	0x00000002

struct msgvp_85xx {
	u32	mivpr0;
	u8	res0[12];
	u32	midr0;
	u8	res1[12];
	u32	mivpr1;
	u8	res2[12];
	u32	midr1;
	u8	res3[12];
	u32	mivpr2;
	u8	res4[12];
	u32	midr2;
	u8	res5[12];
	u32	mivpr3;
	u8	res6[12];
	u32	midr3;
	u8	res7[1420];
};

#ifdef BOARDNET_NDEBUG
# define assert(expr) do {} while (0)
#else
# define assert(expr) \
	if (!(expr)) {							\
	printk(KERN_DEBUG "Assertion failed! %s,%s,%s,line=%d\n",	\
		#expr, __FILE__, __FUNCTION__, __LINE__);		\
	}
#endif

/* These are the flags in the message register */
/* tx, rx and device flags */
#define	AGENT_SENT		0x00000001
#define	AGENT_GET		0x00000000
#define	HOST_SENT		0x00000001
#define HOST_GET		0x00000000
#define	DEV_TBUSY		0x00000001

/* Define max packet buffer */
#define	MAX_PACKET_BUF		(2*1024)
#define NEED_LOCAL_PAGE		0

/* Default timeout period */
#define	BOARDNET_TIMEOUT	5	/* In jiffies */

struct base_addr_reg {
	uint32_t start;
	uint32_t end;
	uint32_t len;
	uint32_t flags;
};

struct pci_agent_dev {
	u32	local_addr;
	u32	mem_addr;
	u32	mem_size;
	u32	pci_addr;
	u32	window_num;
	u32	irq;
	u32	message;
};

extern int setup_agent_inbound_window(struct pci_agent_dev *dev);
extern int setup_host_inbound_window(int);
extern int setup_agent_outbound_window(int, int);
extern int ppc85xx_interrupt_init(void __iomem *);
extern int ppc85xx_clean_interrupt(void __iomem *);
extern int ppc85xx_trigger_intr(u32, void __iomem *);
extern int ppc85xx_readmsg(u32 *, void __iomem *);

#endif
