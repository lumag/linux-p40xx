/*
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xiaobo Xie <r63061@freescale.com>
 *
 * Description:
 * PCI Agent Ethernet Driver for Freescale PowerPC85xx Processor
 * PCIE EP Ethernet Driver for Freescale PowerPC85xx Processor
 *
 * Changelog:
 * Mon Mar. 10 2008 Roy Zang <tie-fei.zang@freescale.com>
 * - Add PCI Express EP support
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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/delay.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/of_platform.h>

#include <sysdev/fsl_soc.h>
#include "pci_agent_lib.h"

/*
 * Debug switch
 */

#define DEBUG
#ifdef DEBUG
  #define DPRINTK(fmt, args...)   printk(KERN_DEBUG "%s: " fmt, \
					__FUNCTION__ , ##args)
#else
  #define DPRINTK(fmt, args...)
#endif

struct share_mem {
	u32 hstatus;
	u32 astatus;

	u32 tx_flags;
	u32 tx_packetlen;
	u8 txbuf[MAX_PACKET_BUF - 12];

	u32 rx_flags;
	u32 rx_packetlen;
	u8 rxbuf[MAX_PACKET_BUF - 12];
};

struct boardnet_private {
	u32 m_immrbar;
	void *m_ioaddr;
	void __iomem *mesgu;
	void *dmabase;
	struct net_device_stats stats;
	struct pci_dev *pci_dev;
	u32 local_mem_phy;
	u32 phy_ioaddr;
	u32 size;

	int pci_express;

	int irq;
	spinlock_t lock; /* lock for set private data */

	struct sk_buff *skb;
};

static int boardnet_open(struct net_device *dev);
static int boardnet_release(struct net_device *dev);
static int boardnet_config(struct net_device *dev, struct ifmap *map);
static void boardnet_hw_tx(char *buf, int len, struct net_device *dev);
static int boardnet_start_xmit(struct sk_buff *skb, struct net_device *dev);
static void boardnet_rx(struct net_device *dev);
static void boardnet_tx_timeout(struct net_device *dev);
static int boardnet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static struct net_device_stats *boardnet_get_stats(struct net_device *dev);
static int boardnet_rebuild_header(struct sk_buff *skb);
static int boardnet_header(struct sk_buff *skb, struct net_device *dev,
			unsigned short type, void *daddr, const void *saddr,
			unsigned int len);
static int boardnet_change_mtu(struct net_device *dev, int new_mtu);
static irqreturn_t boardnet_interrupt(int irq, void *dev_id);

static int eth;

static inline void cache_flush(void *addr)
{
	asm volatile("dcbf %0, %1" : : "r"(0), "r"((uint32_t)addr));
}

/*
 * Open: Init status, register irq and enable start
 */
static int boardnet_open(struct net_device *dev)
{
	int retval;
	retval = request_irq(dev->irq, boardnet_interrupt, IRQF_SHARED,
				dev->name, dev);
	if (retval)
		return retval;

	/*
	* Assign the hardware address of the board: use "\0FSLD1"
	*/

	netif_start_queue(dev);

	printk(KERN_INFO "%s is up\n", dev->name);
	return 0;
}

/*
 * Close: Undo all done in open function
 */
static int boardnet_release(struct net_device *dev)
{

	netif_stop_queue(dev); /* can't transmit any more */

	synchronize_irq(dev->irq);
	free_irq(dev->irq, dev);
	printk(KERN_INFO "%s is down\n", dev->name);
	return 0;
}

/*
 * Configuration changes (passed on by ifconfig)
 */
static int boardnet_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "Methernet: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq) {
		dev->irq = map->irq;
		/* request_irq() is delayed to open-time */
	}
	return 0;
}

/*
 * Receive a packet: retrieve, encapsulate and pass over
 * to upper levels
 */
static void boardnet_rx(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	int len = 0;
	struct sk_buff *skb;
	u32 *src;
	u32 *dest;
	u32 skblen, temp;

	len = shmem->rx_packetlen;
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		DPRINTK("Methernet: Hmm... packet is too short (%i octets)\n",
			shmem->rx_packetlen);
		shmem->hstatus = HOST_GET;
		priv->stats.rx_errors++;
		return;
	}

	/*
	* The packet has been retrieved from the transmission
	* medium. Build an skb around it, so upper layers can handle it
	*/
	skb = dev_alloc_skb(len + 2);
	if (!skb) {
		printk(KERN_WARNING
			"Methernet rx: low on mem-packet dropped\n");
		priv->stats.rx_dropped++;
		return;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */

	dest = (u32 *)skb_put(skb, len);
	src = (u32 *)shmem->rxbuf;
	skblen = len;

	while (skblen > 0) {
		if (skblen < 4) {
			temp = in_be32(src);
			memcpy(dest, &(temp), skblen);
			break;
		} else {
			*dest = in_be32(src);
		}
		src++;
		dest++;
		skblen = skblen - 4;
	}

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += len;

	shmem->hstatus = HOST_GET;
	netif_rx(skb);
	return;
}

/*
 * The interrupt entry point
 */
static irqreturn_t boardnet_interrupt(int irq, void *dev_id)
{
	uint32_t statusword;

	struct net_device *dev = (struct net_device *)dev_id;
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *smem;

	if (!dev)
		return IRQ_NONE;

	smem = (struct share_mem *)priv->m_ioaddr;

	/* Lock the device */
	spin_lock(&priv->lock);
	ppc85xx_readmsg(&statusword, priv->mesgu);
	if (!(statusword & AGENT_SENT)) {
		spin_unlock(&priv->lock);
		return IRQ_NONE;
	}
	if (smem->hstatus != HOST_GET)
		boardnet_rx(dev);

	spin_unlock(&priv->lock);
	return IRQ_HANDLED;
}

/*
 * Transmit a packet (low level interface)
 */
static void boardnet_hw_tx(char *buf, int len, struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	u32 message;
	u32 *src;
	u32 *dest;
	u32 skblen, temp;

	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		DPRINTK("Methernet: packet is too short (%i octets)\n", len);
		priv->stats.tx_errors++;
		dev_kfree_skb(priv->skb);
		return;
	}

	if (len > 2032) {
		printk(KERN_INFO "packet too long (%i octets)\n", len);
		priv->stats.tx_dropped++;
		dev_kfree_skb(priv->skb);
		return;
	}

	/* Send out the packet */
	src = (u32 *)buf;
	dest = (u32 *)shmem->txbuf;
	skblen = len;

	while (skblen > 0) {
		if (skblen < 4) {
			memcpy(&(temp), src, skblen);
			out_be32(dest, temp);
			break;
		} else {
			out_be32(dest, *src);
		}
		src++;
		dest++;
		skblen = skblen - 4;
	}

	shmem->tx_packetlen = len;

	/* Update the statitic data */
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += len;

	/* Set the flag, indicating the peer that the packet has been sent */
	shmem->astatus |= HOST_SENT;

	message = HOST_SENT;
	ppc85xx_trigger_intr(message,
			(void __iomem *)(priv->m_immrbar + MPC85xx_MSG_OFFSET));

	dev_kfree_skb(priv->skb);
	return;
}

/*
 * Transmit a packet (called by the kernel)
 * This is called by the kernel when a packet is ready for transmission
 */
static int boardnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	struct share_mem *shmem = priv->m_ioaddr;
	char *data, shortpkt[ETH_ZLEN];
	int time_out = 100;
	int len;

	while (shmem->astatus & DEV_TBUSY) {
		udelay(2);
		time_out--;
		if (!time_out) {
			DPRINTK("tint for %p, tbusy, skb %p\n", dev, skb);
			netif_stop_queue(dev);
			priv->stats.tx_dropped++;
			dev_kfree_skb(skb);
			return 0;
		}
	}

	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	dev->trans_start = jiffies; /* save the timestamp */

	/* Remember the skb, so we can free it at interrupt time */
	priv->skb = skb;

	/* actual deliver of data is device-specific, and not shown here */
	boardnet_hw_tx(data, len, dev);
	return 0;
}

/*
 * Deal with a transmit timeout.
 */
static void boardnet_tx_timeout(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);

	/* discard the unsent packet */
	dev_kfree_skb(priv->skb);

	netif_wake_queue(dev);
	return;
}

/*
 * Ioctl commands
 */
static int boardnet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	DPRINTK("ioctl\n");
	return 0;
}

/*
 * This is called when ifconfig is issueed
 * Return statistics to the caller
 */
static struct net_device_stats *boardnet_get_stats(struct net_device *dev)
{
	struct boardnet_private *priv = netdev_priv(dev);
	return &priv->stats;
}

/*
 * This function is called to fill up an eth header, since arp is not
 * available on the interface
 */
static int boardnet_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */
	return 0;
}

/*
 * This function is called to fill up an eth header, since arp is not
 * available on the interface
 */
static int
boardnet_header(struct sk_buff *skb, struct net_device *dev,
		unsigned short type, void *daddr, const void *saddr,
		unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb, ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01; /* dest is us xor 1 */

	return dev->hard_header_len;
}

/*
 * The "change_mtu" method is usually not needed.
 * If you need it, it must be like this.
 */
static int boardnet_change_mtu(struct net_device *dev, int new_mtu)
{
	struct boardnet_private *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;
	unsigned long flags;

	/* check ranges */
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;
	/*
	 * Do anything you need, and the accept the value
	 */
	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	return 0; /* success */
}

/*
 * Cleanup
 */
static void boardnet_cleanup(struct pci_dev *pdev)
{
	struct net_device *dev;
	struct boardnet_private *priv;

	dev = pci_get_drvdata(pdev);
	priv = netdev_priv(dev);
	/* Unmap the space address */
	iounmap((void *)(priv->m_immrbar));
	iounmap((void *)(priv->m_ioaddr));
	iounmap((void *)(priv->mesgu));
	free_netdev(dev);
	/* Release region */
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	return;
}

/*
 * Called when remove the device
 */
static __devexit void boardnet_remove(struct pci_dev *pdev)
{
	struct net_device *dev;

	dev = pci_get_drvdata(pdev);
	unregister_netdev(dev);
	boardnet_cleanup(pdev);
	/* Clear the device pointer in PCI */
	pci_set_drvdata(pdev, NULL);
}

/*
 * Called to initialize the board
 */
static __devinit int
boardnet_board_init(struct pci_dev *pdev, struct net_device **dev_out)
{
	struct net_device *dev;
	struct boardnet_private *priv;
	struct base_addr_reg immrbar;
	uint32_t localaddr, pciaddr, size;
	void *mapped_immrbar;
	int retval;

	if (pdev->vendor == PCI_VENDOR_ID_FREESCALE &&
		pdev->device == PCI_DEVICE_ID_MPC8568E)
		DPRINTK("Vendor: Freescale\tDevice: MPC8568E\n");

	/* Enable device */
	retval = pci_enable_device(pdev);
	if (retval) {
		printk(KERN_ERR "Cannot enable device\n");
		return retval;
	}

	/* Read information from base register */
	immrbar.start	= pci_resource_start(pdev, 0);
	immrbar.end	= pci_resource_end(pdev, 0);
	immrbar.flags	= pci_resource_flags(pdev, 0);
	immrbar.len	= pci_resource_len(pdev, 0);

	/* Make sure region #0 is memory space */
	if (!(immrbar.flags & IORESOURCE_MEM)) {
		printk(KERN_ERR "Region #0 is not memory space\n");
		pci_disable_device(pdev);
		retval = -ENODEV;
	}
	/* Make sure region #0 size is 1MB */
	if (immrbar.len < IMMRBAR_SIZE) {
		printk(KERN_ERR "%s: Invalid PCI memregion size(s),aborting\n",
			pci_name(pdev));
		pci_disable_device(pdev);
		retval = -ENODEV;
	}

	/* Mark the memory region used by Methernet */
	retval = pci_request_regions(pdev, "boardnet");
	if (retval) {
		printk(KERN_ERR "%s: Cannot reserve region, aborting\n",
			pci_name(pdev));
		pci_disable_device(pdev);
		return -ENODEV;

	}

	/* Enable PCI bus mastering */
	pci_set_master(pdev);
	/* ioremap address */
	mapped_immrbar = ioremap(immrbar.start, immrbar.len);
	if (mapped_immrbar == NULL) {
		printk(KERN_ERR "%s: Cannot remap memory, aborting\n",
			pci_name(pdev));
		pci_release_regions(pdev);
		pci_disable_device(pdev);
		retval = -EIO;
	}

	/* Setup outbound window 1 in host */
	localaddr = HOST_LOCAL_SPACE_ADDR;
	size = AGENT_MEM_SIZE;
	pciaddr = PCI_SPACE_ADDRESS;

	*dev_out = NULL;
	/*
	 * Allocate and set up an ethernet device.
	 * dev and dev->priv zeroed in alloc_etherdev.
	 */
	dev = alloc_etherdev(sizeof(struct boardnet_private));
	if (dev == NULL) {
		printk(KERN_ERR PFX "%s: Unable to alloc new net device\n",
			pci_name(pdev));
		pci_release_regions(pdev);
		pci_disable_device(pdev);
		return -ENOMEM;
	}

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct boardnet_private));

	/* Init private data structure */
	/* for pci_express currently */
	priv->pci_express = 1;
	priv->pci_dev = pdev;
	priv->m_immrbar = (uint32_t)mapped_immrbar;
	priv->mesgu = ioremap(get_immrbase() +
				MPC85xx_MSG_OFFSET,
				MPC85xx_MSG_SIZE);


	priv->phy_ioaddr = (uint32_t)pci_resource_start(pdev, 1);
	priv->size = AGENT_MEM_SIZE;
	priv->m_ioaddr = ioremap(priv->phy_ioaddr, AGENT_MEM_SIZE);

	*dev_out = dev;
	return 0;
}

static const struct header_ops boardnet_header_ops = {
	.create =  boardnet_header,
	.rebuild = boardnet_rebuild_header,
};
/*
 * The probe function (often called init).
 * It is invoked by pci_module_init()
 */
static int __devinit boardnet_probe(struct pci_dev *pdev,
					const struct pci_device_id *id)
{
	struct net_device *dev = NULL;
	struct boardnet_private *priv;
	int timeout = BOARDNET_TIMEOUT;
	int boardnet_eth = eth;
	int retval;

	assert(pdev != NULL);
	assert(id != NULL);

	/* when we're built into the kernel, the driver version message
	 * is only printed if at least one mpc8540eval board has been found
	 */
#ifndef MODULE
	{
		static int printed_version;
		if (!printed_version++)
			printk(KERN_INFO PPC85XX_NETDRV_NAME "\n");
	}
#endif

	/*
	 * Make the usual checks: check_region(), probe irq, ...  -ENODEV
	 * should be returned if no device found. No resource should be
	 * grabbed: this is done on open().
	 */
	retval = boardnet_board_init(pdev, &dev);
	if (retval)
		return retval;

	assert(dev != NULL);
	priv = netdev_priv(dev);
	assert(priv != NULL);

	setup_host_inbound_window(priv->pci_express);

	if (!boardnet_eth)
		strcpy(dev->name, "beth%d");
	else
		strcpy(dev->name, "eth%d");

	dev->open		= boardnet_open;
	dev->stop		= boardnet_release;
	dev->set_config		= boardnet_config;
	dev->hard_start_xmit	= boardnet_start_xmit;
	dev->do_ioctl		= boardnet_ioctl;
	dev->get_stats		= boardnet_get_stats;
	dev->change_mtu		= boardnet_change_mtu;
	dev->tx_timeout		= boardnet_tx_timeout;
	dev->watchdog_timeo	= timeout;
	dev->header_ops		= &boardnet_header_ops;

	/* keep the default flags, just add NOARP */
	dev->flags		|= IFF_NOARP;
	dev->features		|= NETIF_F_NO_CSUM;
	dev->irq		= ppc85xx_interrupt_init(priv->mesgu);

	memcpy(dev->dev_addr, "\0FSLD1", ETH_ALEN);
	/* initialize lock */
	spin_lock_init(&(priv->lock));

	/* dev is fully set up and ready to use now */
	retval = register_netdev(dev);
	if (retval) {
		printk(KERN_ERR "%s: Cannot register device, aborting\n",
			dev->name);
		boardnet_cleanup(pdev);
		return retval;
	}

	/* To remember netdev pointer in PCI */
	printk(KERN_INFO "register device named-----%s\n", dev->name);
	pci_set_drvdata(pdev, dev);
	return 0;
}

/*
 * Suspend
 */
static int boardnet_suspend(struct pci_dev *pdev, pm_message_t state)
{
	return 0;
}

/*
 * Resume from suspend
 */
static int boardnet_resume(struct pci_dev *pdev)
{
	return 0;
}

/*
 * List devices that this driver support
 */
static struct pci_device_id boardnet_id_table[] = {
	/* Vendor_id, Device_id
	* Subvendor_id, Subdevice_id
	* Class_id, Class_mask
	* Driver_data
	*/
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_MPC8568E, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{PCI_VENDOR_ID_FREESCALE, PCI_DEVICE_ID_P4080, PCI_ANY_ID,
			PCI_ANY_ID, 0x0b2001, 0xffffff, 0},
	{0,}
};
MODULE_DEVICE_TABLE(pci, boardnet_id_table);

/*
 * PCI Device info
 */
static struct pci_driver boardnet_pci_driver = {
	.name		= PPC85XX_NETDRV_NAME,
	.id_table	= boardnet_id_table,
	.probe		= boardnet_probe,
	.remove		= __devexit_p(boardnet_remove),
	.suspend	= boardnet_suspend,
	.resume		= boardnet_resume
};

/*
 * Entry to insmod this driver module
 */
static int __init boardnet_pci_init(void)
{
	int retval;

	retval = pci_register_driver(&boardnet_pci_driver);
	if (retval) {
		printk(KERN_ERR "MPC85xx agent-net drvier init fail."
				"ret = %d\n", retval);
		return retval;
	}
	printk("MPC85xx agent-net drvier init succeed\n");
	return 0;
}

/*
 * Entry to rmmod this driver module
 */
static void __exit boardnet_pci_exit(void)
{
	pci_unregister_driver(&boardnet_pci_driver);
	return;
}

module_init(boardnet_pci_init);
module_exit(boardnet_pci_exit);

MODULE_AUTHOR("Xiaobo Xie<X.Xie@freescale.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MPC85xx Processor PCI Agent Ethernet Driver");
