/*
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <X.Xie@freescale.com>
 *
 * Description:
 * PCI Agent/PCI-E EP Ethernet Driver for Freescale PowerPC85xx Processor
 *
 * Changelog:
 * Jan. 2009 Roy Zang <tie-fei.zang@freescale.com>
 * - Add PCI Express EP support for P4080
 *
 * This file is part of the Linux Kernel
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/in6.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>

#include <asm/checksum.h>
#include <asm/page.h>
#include <sysdev/fsl_soc.h>

#include "pci_agent_lib.h"

static int eth;
module_param(eth, int, 0);

#define CARD_TIMEOUT 10
#define HAVE_TX_TIMEOUT
#ifdef HAVE_TX_TIMEOUT
static int timeout = CARD_TIMEOUT;
module_param(timeout, int, 0);
#endif

#define NEED_LOCAL_PAGE		0

#define	CTL_STATUS_SIZE	24
#define	RX_SPACE_SIZE	(2*1024-12)
#define	TX_SPACE_SIZE	(2*1024-12)
#define	AGENT_DRIVER_MEM_SIZE	(CTL_STATUS_SIZE+RX_SPACE_SIZE+TX_SPACE_SIZE)

struct cardnet_share_mem {
	u32	hstatus;
	u32	astatus;

	u32	rx_flags;
	u32	rx_packetlen;
	u8	rxbuf[2*1024 - 12];

	u32	tx_flags;
	u32	tx_packetlen;
	u8	txbuf[2*1024 - 12];
};

struct card_priv {
	struct cardnet_share_mem *share_mem;
	void __iomem *mesgu;
	void __iomem *h_msg;

	struct sk_buff *cur_tx_skb;
	int rx_packetlen;
	int tx_packetlen;
	u32 ccsrbar;
	int  pci_express;
	spinlock_t lock; /* lock for set card_priv */
	struct net_device_stats stats;
	struct pci_agent_dev *pcidev;
};

static void card_tx_timeout(struct net_device *dev);
static irqreturn_t card_interrupt(int irq, void *dev_id);

static inline void pci_agent_cache_flush(void *addr)
{
	asm volatile("dcbf %0, %1" : : "r"(0), "r"((uint32_t)addr));
}

/*
 * Open and close
 */
static int card_open(struct net_device *dev)
{
	struct card_priv *tp = dev->priv;
	int retval;
	u32 phys_addr;
	struct pci_agent_dev *pci_dev;

	tp->mesgu = ioremap(get_immrbase() + MPC85xx_MSG_OFFSET,
				MPC85xx_MSG_SIZE);
	dev->irq = ppc85xx_interrupt_init(tp->mesgu);
	retval = request_irq(dev->irq, card_interrupt, IRQF_SHARED,
					dev->name, dev);
	if (retval) {
		printk(KERN_ERR "EXIT, returning %d\n", retval);
		return retval;
	}

	tp->share_mem = (struct cardnet_share_mem *)
				__get_free_pages(GFP_ATOMIC, NEED_LOCAL_PAGE);
	if (tp->share_mem == NULL)
		return -ENOMEM;

	phys_addr = virt_to_phys((void *)tp->share_mem);

	memset(tp->share_mem, 0, PAGE_SIZE << NEED_LOCAL_PAGE);
	tp->pcidev = (struct pci_agent_dev *)
			kmalloc(sizeof(struct pci_agent_dev), GFP_KERNEL);
	if (tp->pcidev == NULL)
		return -ENOMEM;

	pci_dev = tp->pcidev;
	memset(pci_dev, 0, sizeof(struct pci_agent_dev));
	pci_dev->mem_addr = (u32)tp->share_mem;
	pci_dev->local_addr = phys_addr;
	pci_dev->pci_addr = PCI_SPACE_ADDRESS;
	pci_dev->mem_size = AGENT_DRIVER_MEM_SIZE;
	pci_dev->window_num = 1;

	/*Setup inbound window in agent*/
	retval = setup_agent_inbound_window(pci_dev);

	if (retval == 1)
		tp->pci_express = 1;

	/*Setup outbound window in agent*/
	setup_agent_outbound_window(tp->pci_express, 1);

	if (check_mem_region(pci_dev->pci_addr, AGENT_DRIVER_MEM_SIZE)) {
		printk(KERN_INFO "cardnet:memory already in use!\n");
		return -EBUSY;
	}
	request_mem_region(pci_dev->pci_addr, AGENT_DRIVER_MEM_SIZE,
				"cardnet");

	if (tp->pci_express)
		tp->h_msg = (struct mesg_85xx *)ioremap(PCIE_CCSR_BAR +
						MPC85xx_MSG_OFFSET,
						MPC85xx_MSG_SIZE);
	else
		tp->h_msg = (struct mesg_85xx *)ioremap(PCI_CCSR_BAR +
						MPC85xx_MSG_OFFSET,
						MPC85xx_MSG_SIZE);

	dev->mem_start = phys_addr;
	dev->mem_end = phys_addr + AGENT_DRIVER_MEM_SIZE;
	dev->base_addr = pci_dev->pci_addr;

	/*
	 * Assign the hardware address of the agent: use "\0FSLD0",
	 * The hardware address of the host use "\0FSLD1";
	 */
	printk(KERN_INFO "%s device is up\n", dev->name);

	netif_start_queue(dev);
	return 0;
}

static int card_release(struct net_device *dev)
{
	struct card_priv *tp = dev->priv;
	struct cardnet_share_mem *share_mem = \
			(struct cardnet_share_mem *)tp->share_mem;
	struct pci_agent_dev *pci_dev;
	pci_dev = tp->pcidev;

	free_pages((unsigned long)share_mem, NEED_LOCAL_PAGE);
	/* release ports, irq and such*/
	release_mem_region(pci_dev->pci_addr, AGENT_DRIVER_MEM_SIZE);
	kfree(tp->pcidev);
	ppc85xx_clean_interrupt(tp->mesgu);
	iounmap(tp->mesgu);
	iounmap(tp->h_msg);

	/* release the irq */
	synchronize_irq(dev->irq);
	free_irq(dev->irq, dev);
	netif_stop_queue(dev); /* can't transmit any more */

	return 0;
}

/*
 * Configuration changes (passed on by ifconfig)
 */
static int card_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;
	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "cardnet: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}
	/* Allow changing the IRQ */
	if (map->irq != dev->irq)
		dev->irq = map->irq;

	/* ignore other fields */
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over to upper levels
 */
static void card_rx(struct net_device *dev, int len, unsigned char *buf)
{
	struct card_priv *priv = (struct card_priv *) dev->priv;
	struct sk_buff *skb;

	skb = dev_alloc_skb(len+2);
	if (!skb) {
		printk(KERN_ERR "card rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		return;
	}

	skb_reserve(skb, 2);
	memcpy(skb_put(skb, len), buf, len);
	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += len;

	netif_rx(skb);
	return;
}

/*
 * The interrupt entry point
 */
static irqreturn_t card_interrupt(int irq, void *dev_id)
{
	struct card_priv *priv;
	struct cardnet_share_mem *shmem;
	u32  statusword = 0;
	int len;
	struct net_device *dev = (struct net_device *)dev_id;

	if (!dev)
		return IRQ_NONE;
	priv = (struct card_priv *) dev->priv;

	/* Lock the device */
	spin_lock(&priv->lock);
	shmem = (struct cardnet_share_mem *) priv->share_mem;

	ppc85xx_readmsg(&statusword, priv->mesgu);

	if (statusword & HOST_SENT) {
		len = shmem->rx_packetlen;
		card_rx(dev, len, shmem->rxbuf);
		shmem->astatus = AGENT_GET;
	} else {
		printk(KERN_INFO "The message is not for me!message=0x%x\n",
					statusword);
		spin_unlock(&priv->lock);
		return IRQ_NONE;
	}

	/* Unlock the device and we are done */
	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

/*
 * Transmit a packet (called by the kernel)
 */
static int card_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	int j = 0, count = 0;
	char *data;
	struct card_priv *priv = (struct card_priv *) dev->priv;
	struct cardnet_share_mem *shmem =
				(struct cardnet_share_mem *)priv->share_mem;

	if (skb == NULL) {
		printk(KERN_ERR "skb is NULL\n");
		return 0;
	}

	len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk(KERN_INFO "packet too short (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return 0;
	}
	if (len > 2032) {
		printk(KERN_INFO "packet too long (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return 0;
	}

	spin_lock(&priv->lock);

	data = skb->data;
	dev->trans_start = jiffies; /* save the timestamp */
	priv->cur_tx_skb = skb;
	priv->tx_packetlen = len;

	while (shmem->hstatus) {
		udelay(20); j++;
		if (j > 1000)
			break;
	}
	if (j > 1000) {
		netif_stop_queue(dev);
		priv->stats.tx_dropped++;
		dev_kfree_skb(priv->cur_tx_skb);
		spin_unlock(&priv->lock);
		return 0;
	}

	shmem->tx_flags = ++count;
	shmem->tx_packetlen = len;
	memcpy(shmem->txbuf, data, len);
	shmem->hstatus = AGENT_SENT;

	dev_kfree_skb(priv->cur_tx_skb);
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += priv->tx_packetlen;

	ppc85xx_trigger_intr(AGENT_SENT, priv->h_msg);
	spin_unlock(&priv->lock);
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
static void card_tx_timeout(struct net_device *dev)
{
	struct card_priv *priv = (struct card_priv *) dev->priv;

	printk(KERN_INFO "Transmit timeout at %ld, latency %ld\n",
		jiffies, jiffies - dev->trans_start);

	ppc85xx_clean_interrupt(priv->h_msg);

	/*When timeout, try to kick the EP*/
	ppc85xx_trigger_intr(HOST_SENT, priv->mesgu);
	priv->stats.tx_errors++;
	netif_wake_queue(dev);
	return;
}

/*
 * Ioctl commands
 */
static int card_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	printk(KERN_DEBUG "ioctl\n");
	return 0;
}

static struct net_device_stats *card_stats(struct net_device *dev)
{
	struct card_priv *priv = (struct card_priv *) dev->priv;
	return &priv->stats;
}

static int card_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */

	return 0;
}

static int card_header(struct sk_buff *skb, struct net_device *dev,
		unsigned short type, void *daddr, const void *saddr,
		unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest,   dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01;
	return dev->hard_header_len;
}

static int card_change_mtu(struct net_device *dev, int new_mtu)
{
	spinlock_t *lock = &((struct card_priv *) dev->priv)->lock;
	unsigned long flags;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

static const struct header_ops card_header_ops = {
	.create = card_header,
	.rebuild = card_rebuild_header,
};

/*
 * init function.
 */
static void card_init(struct net_device *dev)
{
	struct card_priv *priv;
	ether_setup(dev); /* assign some of the fields */
	dev->open		= card_open;
	dev->stop		= card_release;
	dev->set_config		= card_config;
	dev->hard_start_xmit	= card_tx;
	dev->do_ioctl		= card_ioctl;
	dev->get_stats		= card_stats;
	dev->change_mtu		= card_change_mtu;
	dev->header_ops		= &card_header_ops;
	memcpy(dev->dev_addr, "\0FSLD0", ETH_ALEN);

#ifdef HAVE_TX_TIMEOUT
	dev->tx_timeout		= card_tx_timeout;
	dev->watchdog_timeo	= 2 * HZ;
#endif
	/* keep the default flags, just add NOARP */
	dev->flags	|= IFF_NOARP;
	dev->features	|= NETIF_F_NO_CSUM;

	/*
	 * Then, allocate the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct card_priv));

	spin_lock_init(&((struct card_priv *) dev->priv)->lock);
	return;
}

/*
 * The devices
 */
static struct net_device *card_devs;

static __init int card_init_module(void)
{
	int result, device_present = 0;
	int card_eth;
	char interface_name[16];

	card_eth = eth; /* copy the cfg datum in the non-static place */
	if (!card_eth)
		strcpy(interface_name, "ceth%d");
	else
		strcpy(interface_name, "eth%d");

	card_devs = alloc_netdev(sizeof(struct card_priv),
					interface_name, card_init);
	if (card_devs == NULL)
		return -ENODEV;

	result = register_netdev(card_devs);
	if (result) {
		printk(KERN_ERR "card: error %i registering device \"%s\"\n",
			result, interface_name);
		free_netdev(card_devs);
	} else
		device_present++;
	printk(KERN_INFO "register device named-----%s\n", card_devs->name);
	printk(KERN_INFO "mpc85xx agent drvier init succeed\n");

	return device_present ? 0 : -ENODEV;
}

static __exit void card_cleanup(void)
{
	unregister_netdev(card_devs);
	free_netdev(card_devs);
	return;
}
module_init(card_init_module);
module_exit(card_cleanup);

MODULE_AUTHOR("Xiaobo Xie<X.Xie@freescale.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MPC85xx Processor PCI Agent Ethernet Driver");
