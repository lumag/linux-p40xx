/*
 * rionet - Ethernet driver over RapidIO messaging services
 *
 * Copyright (C) 2007, 2009 Freescale Semiconductor, Inc. All rights reserved.
 * Author: Zhang Wei, wei.zhang@freescale.com, Jun 2007
 *
 * Changelog:
 * Jun 2007 Zhang Wei <wei.zhang@freescale.com>
 * - Added the support to RapidIO memory driver. 2007.
 *
 * Copyright 2005 MontaVista Software, Inc.
 * Matt Porter <mporter@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/rio.h>
#include <linux/rio_drv.h>
#include <linux/rio_ids.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/crc32.h>
#include <linux/ethtool.h>
#include <linux/dmaengine.h>

#define DRV_NAME        "rionet"
#define DRV_VERSION     "0.2"
#define DRV_AUTHOR      "Matt Porter <mporter@kernel.crashing.org>"
#define DRV_DESC        "Ethernet over RapidIO"

MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");

#define RIONET_DEFAULT_MSGLEVEL \
			(NETIF_MSG_DRV          | \
			 NETIF_MSG_LINK         | \
			 NETIF_MSG_RX_ERR       | \
			 NETIF_MSG_TX_ERR)

#define RIONET_DOORBELL_JOIN	0x1000
#ifdef CONFIG_RIONET_MEMMAP
#define RIONET_DOORBELL_SEND	0x1001
#define RIONET_DOORBELL_LEAVE	0x1002
#else
#define RIONET_DOORBELL_LEAVE	0x1001
#endif

#define RIONET_MAILBOX		0

#define RIONET_TX_RING_SIZE	CONFIG_RIONET_TX_SIZE
#define RIONET_RX_RING_SIZE	CONFIG_RIONET_RX_SIZE

#define ERR(fmt, arg...) \
	printk(KERN_ERR "ERROR %s - %s: " fmt,  __FILE__, __func__, ## arg)

#ifdef CONFIG_RIONET_MEMMAP
/* Definitions for rionet memory map driver */
#define RIONET_DRVID		0x101
#define RIONET_MAX_SK_DATA_SIZE	0x1000
#define RIONET_MEM_RIO_BASE	0x10000000
#define RIONET_TX_RX_BUFF_SIZE	(0x1000 * (128 + 128))
#define RIONET_QUEUE_NEXT(x)	(((x) < 127) ? ((x) + 1) : 0)
#define RIONET_QUEUE_INC(x)	(x = RIONET_QUEUE_NEXT(x))

struct sk_data {
	u8	data[0x1000];
};

#define RIONET_SKDATA_EN	0x80000000
struct rionet_tx_rx_buff {
	int		enqueue;		/* enqueue point */
	int		dequeue;		/* dequeue point */
	u32		size[128];		/* size[i] is skdata[i] size
						 * the most high bit [31] is
						 * enable bit. The
						 * max size is 4096.
						 */
	u8		rev1[3576];
	struct sk_data	skdata[128];		/* all size are 0x1000 * 128 */
};
#endif /* CONFIG_RIONET_MEMMAP */

static LIST_HEAD(rionet_peers);

struct rionet_private {
	struct rio_mport *mport;
	struct sk_buff *rx_skb[RIONET_RX_RING_SIZE];
	struct sk_buff *tx_skb[RIONET_TX_RING_SIZE];
	int rx_slot;
	int tx_slot;
	int tx_cnt;
	int ack_slot;
	spinlock_t lock;
	spinlock_t tx_lock;
	u32 msg_enable;
#ifdef CONFIG_RIONET_MEMMAP
	struct rionet_tx_rx_buff *rxbuff;
	struct rionet_tx_rx_buff __iomem *txbuff;
	dma_addr_t rx_addr;
	phys_addr_t tx_addr;
	struct resource *riores;
#ifdef CONFIG_RIONET_DMA
	struct dma_chan *txdmachan;
	struct dma_chan *rxdmachan;
	struct dma_client rio_dma_client;
	spinlock_t rio_dma_event_lock;
#endif
#endif
};

struct rionet_peer {
	struct list_head node;
	struct rio_dev *rdev;
	struct resource *res;
};

static int rionet_check = 0;
static int rionet_capable = 1;

/*
 * This is a fast lookup table for translating TX
 * Ethernet packets into a destination RIO device. It
 * could be made into a hash table to save memory depending
 * on system trade-offs.
 */
static struct rio_dev **rionet_active;

#define is_rionet_capable(pef, src_ops, dst_ops)		\
			((pef & RIO_PEF_INB_MBOX) &&		\
			 (pef & RIO_PEF_INB_DOORBELL) &&	\
			 (src_ops & RIO_SRC_OPS_DOORBELL) &&	\
			 (dst_ops & RIO_DST_OPS_DOORBELL))
#define dev_rionet_capable(dev) \
	is_rionet_capable(dev->pef, dev->src_ops, dev->dst_ops)

#define RIONET_MAC_MATCH(x)	(*(u32 *)x == 0x00010001)
#define RIONET_GET_DESTID(x)	(*(u16 *)(x + 4))

#ifndef CONFIG_RIONET_MEMMAP
static int rionet_rx_clean(struct net_device *ndev)
{
	int i;
	int error = 0;
	struct rionet_private *rnet = netdev_priv(ndev);
	void *data;

	i = rnet->rx_slot;

	do {
		if (!rnet->rx_skb[i])
			continue;

		pr_debug("RIONET: rionet_rx_clean slot %d\n", i);
		if (!(data = rio_get_inb_message(rnet->mport, RIONET_MAILBOX)))
			break;

		rnet->rx_skb[i]->data = data;
		skb_put(rnet->rx_skb[i], RIO_MAX_MSG_SIZE);
		rnet->rx_skb[i]->dev = ndev;
		rnet->rx_skb[i]->protocol =
		    eth_type_trans(rnet->rx_skb[i], ndev);
		error = netif_rx(rnet->rx_skb[i]);
		rnet->rx_skb[i] = NULL;

		if (error == NET_RX_DROP) {
			ndev->stats.rx_dropped++;
		} else {
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += RIO_MAX_MSG_SIZE;
		}

	} while ((i = (i + 1) % RIONET_RX_RING_SIZE) != rnet->rx_slot);

	return i;
}
#endif

static void rionet_rx_fill(struct net_device *ndev, int end)
{
	int i;
	struct rionet_private *rnet = netdev_priv(ndev);

	i = rnet->rx_slot;
	do {
		rnet->rx_skb[i] = dev_alloc_skb(RIO_MAX_MSG_SIZE);

		if (!rnet->rx_skb[i])
			break;

#ifndef CONFIG_RIONET_MEMMAP
		rio_add_inb_buffer(rnet->mport, RIONET_MAILBOX,
				   rnet->rx_skb[i]->data);
#endif
	} while ((i = (i + 1) % RIONET_RX_RING_SIZE) != end);

	rnet->rx_slot = i;
}

#ifdef CONFIG_RIONET_MEMMAP
static int rio_send_mem(struct sk_buff *skb,
				struct net_device *ndev, struct rio_dev *rdev)
{
	struct rionet_private *rnet = ndev->priv;
	int enqueue, dequeue;

	if (!rdev)
		return -EFAULT;

	if (skb->len > RIONET_MAX_SK_DATA_SIZE) {
		printk(KERN_ERR "Frame len is more than RIONET max sk_data!\n");
		return -EINVAL;
	}

	rio_map_outb_region(rnet->mport, rdev->destid, rnet->riores,
			rnet->tx_addr, 0);

	enqueue = in_be32(&rnet->txbuff->enqueue);
	dequeue = in_be32(&rnet->txbuff->dequeue);

	if (!(in_be32(&rnet->txbuff->size[enqueue]) & RIONET_SKDATA_EN)
			&& (RIONET_QUEUE_NEXT(enqueue) != dequeue)) {
#ifdef CONFIG_RIONET_DMA
		struct dma_device *dmadev;
		struct dma_async_tx_descriptor *tx;
		dma_cookie_t tx_cookie = 0;

		dmadev = rnet->txdmachan->device;
		tx = dmadev->device_prep_dma_memcpy(rnet->txdmachan,
			(void *)rnet->txbuff->skdata[enqueue].data
			  - (void *)rnet->txbuff + rnet->tx_addr,
			dma_map_single(&ndev->dev, skb->data, skb->len,
			  DMA_TO_DEVICE), skb->len, DMA_CTRL_ACK);
		if (!tx)
			return -EFAULT;
		tx_cookie = tx->tx_submit(tx);

		dma_async_memcpy_issue_pending(rnet->txdmachan);
		while (dma_async_memcpy_complete(rnet->txdmachan,
				tx_cookie, NULL, NULL) == DMA_IN_PROGRESS) ;
#else
		memcpy(rnet->txbuff->skdata[enqueue].data, skb->data, skb->len);
#endif /* CONFIG_RIONET_DMA */
		out_be32(&rnet->txbuff->size[enqueue],
						RIONET_SKDATA_EN | skb->len);
		out_be32(&rnet->txbuff->enqueue,
						RIONET_QUEUE_NEXT(enqueue));
		in_be32(&rnet->txbuff->enqueue);	/* verify read */
	} else if (netif_msg_tx_err(rnet))
		printk(KERN_ERR "rionmet(memmap): txbuff is busy!\n");

	rio_unmap_outb_region(rnet->mport, rnet->tx_addr);
	rio_send_doorbell(rdev, RIONET_DOORBELL_SEND);
	return 0;
}
#endif

static int rionet_queue_tx_msg(struct sk_buff *skb, struct net_device *ndev,
			       struct rio_dev *rdev)
{
	struct rionet_private *rnet = netdev_priv(ndev);

#ifdef CONFIG_RIONET_MEMMAP
	int ret = 0;
	ret = rio_send_mem(skb, ndev, rdev);
	if (ret)
		return ret;
#else
	rio_add_outb_message(rnet->mport, rdev, 0, skb->data, skb->len);
#endif
	rnet->tx_skb[rnet->tx_slot] = skb;

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += skb->len;

	if (++rnet->tx_cnt == RIONET_TX_RING_SIZE)
		netif_stop_queue(ndev);

	++rnet->tx_slot;
	rnet->tx_slot &= (RIONET_TX_RING_SIZE - 1);

#ifdef CONFIG_RIONET_MEMMAP
	while (rnet->tx_cnt && (rnet->ack_slot != rnet->tx_slot)) {
		/* dma unmap single */
		dev_kfree_skb_any(rnet->tx_skb[rnet->ack_slot]);
		rnet->tx_skb[rnet->ack_slot] = NULL;
		++rnet->ack_slot;
		rnet->ack_slot &= (RIONET_TX_RING_SIZE - 1);
		rnet->tx_cnt--;
	}

	if (rnet->tx_cnt < RIONET_TX_RING_SIZE)
		netif_wake_queue(ndev);
#endif
	if (netif_msg_tx_queued(rnet))
		printk(KERN_INFO "%s: queued skb %8.8x len %8.8x\n", DRV_NAME,
		       (u32) skb, skb->len);

	return 0;
}

static int rionet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int i;
	struct rionet_private *rnet = netdev_priv(ndev);
	struct ethhdr *eth = (struct ethhdr *)skb->data;
	u16 destid;
	unsigned long flags;

	local_irq_save(flags);
	if (!spin_trylock(&rnet->tx_lock)) {
		local_irq_restore(flags);
		return NETDEV_TX_LOCKED;
	}

	if ((rnet->tx_cnt + 1) > RIONET_TX_RING_SIZE) {
		netif_stop_queue(ndev);
		spin_unlock_irqrestore(&rnet->tx_lock, flags);
		printk(KERN_ERR "%s: BUG! Tx Ring full when queue awake!\n",
		       ndev->name);
		return NETDEV_TX_BUSY;
	}

	if (eth->h_dest[0] & 0x01) {
		for (i = 0; i < RIO_MAX_ROUTE_ENTRIES(rnet->mport->sys_size);
				i++)
			if (rionet_active[i])
				rionet_queue_tx_msg(skb, ndev,
						    rionet_active[i]);
	} else if (RIONET_MAC_MATCH(eth->h_dest)) {
		destid = RIONET_GET_DESTID(eth->h_dest);
		if (rionet_active[destid])
			rionet_queue_tx_msg(skb, ndev, rionet_active[destid]);
	}

	spin_unlock_irqrestore(&rnet->tx_lock, flags);

	return NETDEV_TX_OK;
}

#ifdef CONFIG_RIONET_MEMMAP
static void rio_recv_mem(struct net_device *ndev)
{
	struct rionet_private *rnet = (struct rionet_private *)ndev->priv;
	struct sk_buff *skb;
	u32 enqueue, dequeue, size;
	int error = 0;
#ifdef CONFIG_RIONET_DMA
	dma_cookie_t rx_cookie = 0;
	struct dma_device *dmadev;
	struct dma_async_tx_descriptor *tx;
#endif

	dequeue = rnet->rxbuff->dequeue;
	enqueue = rnet->rxbuff->enqueue;

	while (enqueue != dequeue) {
		size = rnet->rxbuff->size[dequeue];
		if (!(size & RIONET_SKDATA_EN))
			return;
		size &= ~RIONET_SKDATA_EN;

		skb = dev_alloc_skb(size + 2);
		if (!skb)
			return;

#ifdef CONFIG_RIONET_DMA
		dmadev = rnet->rxdmachan->device;
		tx = dmadev->device_prep_dma_memcpy(rnet->rxdmachan,
			dma_map_single(&ndev->dev, skb_put(skb, size),
			  size, DMA_FROM_DEVICE),
			(void *)rnet->rxbuff->skdata[dequeue].data
			  - (void *)rnet->rxbuff + rnet->rx_addr,
			size, DMA_CTRL_ACK);
		if (!tx)
			return;
		rx_cookie = tx->tx_submit(tx);
		dma_async_memcpy_issue_pending(rnet->rxdmachan);
		while (dma_async_memcpy_complete(rnet->rxdmachan,
				rx_cookie, NULL, NULL) == DMA_IN_PROGRESS);
#else
		memcpy(skb_put(skb, size),
				rnet->rxbuff->skdata[dequeue].data,
				size);
#endif /* CONFIG_RIONET_DMA */
		skb->dev = ndev;
		skb->protocol = eth_type_trans(skb, ndev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		error = netif_rx(skb);

		rnet->rxbuff->size[dequeue] &= ~RIONET_SKDATA_EN;
		rnet->rxbuff->dequeue = RIONET_QUEUE_NEXT(dequeue);
		dequeue = RIONET_QUEUE_NEXT(dequeue);

		if (error == NET_RX_DROP) {
			ndev->stats.rx_dropped++;
		} else if (error == NET_RX_BAD) {
			if (netif_msg_rx_err(rnet))
				printk(KERN_WARNING "%s: bad rx packet\n",
				       DRV_NAME);
			ndev->stats.rx_errors++;
		} else {
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += RIO_MAX_MSG_SIZE;
		}
	}
}

static void rionet_inb_recv_event(struct rio_mport *mport, void *dev_id)
{
	struct net_device *ndev = dev_id;
	struct rionet_private *rnet = (struct rionet_private *)ndev->priv;
	unsigned long flags;

	if (netif_msg_intr(rnet))
		printk(KERN_INFO "%s: inbound memory data receive event\n",
		       DRV_NAME);

	spin_lock_irqsave(&rnet->lock, flags);
	rio_recv_mem(ndev);
	spin_unlock_irqrestore(&rnet->lock, flags);
}
#endif


static void rionet_dbell_event(struct rio_mport *mport, void *dev_id, u16 sid, u16 tid,
			       u16 info)
{
	struct net_device *ndev = dev_id;
	struct rionet_private *rnet = netdev_priv(ndev);
	struct rionet_peer *peer;

	if (netif_msg_intr(rnet))
		printk(KERN_INFO "%s: doorbell sid %4.4x tid %4.4x info %4.4x",
		       DRV_NAME, sid, tid, info);
	if (info == RIONET_DOORBELL_JOIN) {
		if (!rionet_active[sid]) {
			list_for_each_entry(peer, &rionet_peers, node) {
				if (peer->rdev->destid == sid)
					rionet_active[sid] = peer->rdev;
			}
			rio_mport_send_doorbell(mport, sid,
						RIONET_DOORBELL_JOIN);
		}
	} else if (info == RIONET_DOORBELL_LEAVE) {
		rionet_active[sid] = NULL;
#ifdef CONFIG_RIONET_MEMMAP
	} else if (info == RIONET_DOORBELL_SEND) {
		rionet_inb_recv_event(mport, ndev);
#endif
	} else {
		if (netif_msg_intr(rnet))
			printk(KERN_WARNING "%s: unhandled doorbell\n",
			       DRV_NAME);
	}
}

#ifndef CONFIG_RIONET_MEMMAP
static void rionet_inb_msg_event(struct rio_mport *mport, void *dev_id, int mbox, int slot)
{
	int n;
	struct net_device *ndev = dev_id;
	struct rionet_private *rnet = netdev_priv(ndev);

	if (netif_msg_intr(rnet))
		printk(KERN_INFO "%s: inbound message event, mbox %d slot %d\n",
		       DRV_NAME, mbox, slot);

	spin_lock(&rnet->lock);
	if ((n = rionet_rx_clean(ndev)) != rnet->rx_slot)
		rionet_rx_fill(ndev, n);
	spin_unlock(&rnet->lock);
}

static void rionet_outb_msg_event(struct rio_mport *mport, void *dev_id, int mbox, int slot)
{
	struct net_device *ndev = dev_id;
	struct rionet_private *rnet = netdev_priv(ndev);

	spin_lock(&rnet->lock);

	if (netif_msg_intr(rnet))
		printk(KERN_INFO
		       "%s: outbound message event, mbox %d slot %d\n",
		       DRV_NAME, mbox, slot);

	while (rnet->tx_cnt && (rnet->ack_slot != slot)) {
		/* dma unmap single */
		dev_kfree_skb_irq(rnet->tx_skb[rnet->ack_slot]);
		rnet->tx_skb[rnet->ack_slot] = NULL;
		++rnet->ack_slot;
		rnet->ack_slot &= (RIONET_TX_RING_SIZE - 1);
		rnet->tx_cnt--;
	}

	if (rnet->tx_cnt < RIONET_TX_RING_SIZE)
		netif_wake_queue(ndev);

	spin_unlock(&rnet->lock);
}
#endif

#ifdef CONFIG_RIONET_DMA
static enum dma_state_client rionet_dma_event(struct dma_client *client,
		struct dma_chan *chan, enum dma_state state)
{
	struct rionet_private *rnet = container_of(client,
			struct rionet_private, rio_dma_client);
	enum dma_state_client ack = DMA_DUP;

	spin_lock(&rnet->lock);
	switch (state) {
	case DMA_RESOURCE_AVAILABLE:
		if (!rnet->txdmachan) {
			ack = DMA_ACK;
			rnet->txdmachan = chan;
		} else if (!rnet->rxdmachan) {
			ack = DMA_ACK;
			rnet->rxdmachan = chan;
		}
		break;
	case DMA_RESOURCE_REMOVED:
		if (rnet->txdmachan == chan) {
			ack = DMA_ACK;
			rnet->txdmachan = NULL;
		} else if (rnet->rxdmachan == chan) {
			ack = DMA_ACK;
			rnet->rxdmachan = NULL;
		}
		break;
	default:
		break;
	}
	spin_unlock(&rnet->lock);
	return ack;
}

static int rionet_dma_register(struct rionet_private *rnet)
{
	int rc = 0;
	spin_lock_init(&rnet->rio_dma_event_lock);
	rnet->rio_dma_client.event_callback = rionet_dma_event;
	dma_cap_set(DMA_MEMCPY, rnet->rio_dma_client.cap_mask);
	dma_async_client_register(&rnet->rio_dma_client);
	dma_async_client_chan_request(&rnet->rio_dma_client);

	if (!rnet->txdmachan || !rnet->rxdmachan)
		rc = -ENODEV;

	return rc;
}
#endif

static int rionet_open(struct net_device *ndev)
{
	int i, rc = 0;
	struct rionet_peer *peer, *tmp;
	u32 pwdcsr;
	struct rionet_private *rnet = netdev_priv(ndev);
	unsigned long flags;

	if (netif_msg_ifup(rnet))
		printk(KERN_INFO "%s: open\n", DRV_NAME);

	if ((rc = rio_request_inb_dbell(rnet->mport,
					(void *)ndev,
					RIONET_DOORBELL_JOIN,
					RIONET_DOORBELL_LEAVE,
					rionet_dbell_event)) < 0)
		return rc;

#ifdef CONFIG_RIONET_MEMMAP
	if (!request_rio_region(RIONET_MEM_RIO_BASE, RIONET_TX_RX_BUFF_SIZE,
				ndev->name)) {
		dev_err(&ndev->dev, "RapidIO space busy\n");
		rc = -EBUSY;
		goto out1;
	}

	rnet->riores = kmalloc(sizeof(struct resource), GFP_KERNEL);
	if (!rnet->riores) {
		rc = -ENOMEM;
		goto out2;
	}
	rnet->riores->start = RIONET_MEM_RIO_BASE;
	rnet->riores->end = RIONET_MEM_RIO_BASE + RIONET_TX_RX_BUFF_SIZE - 1;
	rnet->rxbuff = dma_alloc_coherent(&ndev->dev, RIONET_TX_RX_BUFF_SIZE,
			&rnet->rx_addr, GFP_KERNEL);
	if (!rnet->rxbuff) {
		rc = -ENOMEM;
		goto out3;
	}
	rc = rio_map_inb_region(rnet->mport, rnet->riores, rnet->rx_addr, 0);
	if (rc) {
		rc = -EBUSY;
		goto out4;
	}

	/* Use space right after the doorbell window, aligned to
	 * size of RIONET_TX_RX_BUFF_SIZE */
	rnet->tx_addr = rnet->mport->iores.start + 0x500000;
	rnet->txbuff = ioremap(rnet->tx_addr, resource_size(rnet->riores));
	if (!rnet->txbuff) {
		rc = -ENOMEM;
		goto out5;
	}
#ifdef CONFIG_RIONET_DMA
	rc = rionet_dma_register(rnet);
	if (rc)
		goto out6;
#endif /* CONFIG_RIONET_DMA */
#else
	if ((rc = rio_request_inb_mbox(rnet->mport,
				       (void *)ndev,
				       RIONET_MAILBOX,
				       RIONET_RX_RING_SIZE,
				       rionet_inb_msg_event)) < 0)
		goto out1;

	if ((rc = rio_request_outb_mbox(rnet->mport,
					(void *)ndev,
					RIONET_MAILBOX,
					RIONET_TX_RING_SIZE,
					rionet_outb_msg_event)) < 0)
		goto out8;
#endif

	/* Initialize inbound message ring */
	for (i = 0; i < RIONET_RX_RING_SIZE; i++)
		rnet->rx_skb[i] = NULL;
	rnet->rx_slot = 0;
	spin_lock_irqsave(&rnet->lock, flags);
	rionet_rx_fill(ndev, 0);
	spin_unlock_irqrestore(&rnet->lock, flags);

	rnet->tx_slot = 0;
	rnet->tx_cnt = 0;
	rnet->ack_slot = 0;

	netif_carrier_on(ndev);
	netif_start_queue(ndev);

	list_for_each_entry_safe(peer, tmp, &rionet_peers, node) {
		if (!(peer->res = rio_request_outb_dbell(peer->rdev,
							 RIONET_DOORBELL_JOIN,
							 RIONET_DOORBELL_LEAVE)))
		{
			printk(KERN_ERR "%s: error requesting doorbells\n",
			       DRV_NAME);
			continue;
		}

		/*
		 * If device has initialized inbound doorbells,
		 * send a join message
		 */
		rio_read_config_32(peer->rdev, RIO_WRITE_PORT_CSR, &pwdcsr);
		if (pwdcsr & RIO_DOORBELL_AVAIL)
			rio_send_doorbell(peer->rdev, RIONET_DOORBELL_JOIN);
	}
	return 0;

#ifndef CONFIG_RIONET_MEMMAP
out8:
	rio_release_inb_mbox(rnet->mport, RIONET_MAILBOX);
#else
out6:
	iounmap(rnet->txbuff);
out5:
	rio_unmap_inb_region(rnet->mport, rnet->rx_addr);
out4:
	dma_free_coherent(&ndev->dev, RIONET_TX_RX_BUFF_SIZE,
			rnet->rxbuff, rnet->rx_addr);
	rnet->rxbuff = NULL;
	rnet->txbuff = NULL;
out3:
	kfree(rnet->riores);
out2:
	release_rio_region(RIONET_MEM_RIO_BASE, RIONET_TX_RX_BUFF_SIZE);
#endif
out1:
	rio_release_inb_dbell(rnet->mport, RIONET_DOORBELL_JOIN,
			RIONET_DOORBELL_LEAVE);
	return rc;
}

static int rionet_close(struct net_device *ndev)
{
	struct rionet_private *rnet = netdev_priv(ndev);
	struct rionet_peer *peer, *tmp;
	int i;

	if (netif_msg_ifup(rnet))
		printk(KERN_INFO "%s: close\n", DRV_NAME);

	netif_stop_queue(ndev);
	netif_carrier_off(ndev);

	for (i = 0; i < RIONET_RX_RING_SIZE; i++)
		kfree_skb(rnet->rx_skb[i]);

	list_for_each_entry_safe(peer, tmp, &rionet_peers, node) {
		if (rionet_active[peer->rdev->destid]) {
			rio_send_doorbell(peer->rdev, RIONET_DOORBELL_LEAVE);
			rionet_active[peer->rdev->destid] = NULL;
		}
		rio_release_outb_dbell(peer->rdev, peer->res);
	}

	rio_release_inb_dbell(rnet->mport, RIONET_DOORBELL_JOIN,
			      RIONET_DOORBELL_LEAVE);
#ifdef CONFIG_RIONET_MEMMAP
	rio_unmap_inb_region(rnet->mport, rnet->rx_addr);
	iounmap(rnet->txbuff);
	dma_free_coherent(&ndev->dev, RIONET_TX_RX_BUFF_SIZE,
			rnet->rxbuff, rnet->rx_addr);
	kfree(rnet->riores);
	release_rio_region(RIONET_MEM_RIO_BASE, RIONET_TX_RX_BUFF_SIZE);
	rnet->rxbuff = NULL;
	rnet->txbuff = NULL;
#ifdef CONFIG_RIONET_DMA
	dma_async_client_unregister(&rnet->rio_dma_client);
#endif
#else
	rio_release_inb_mbox(rnet->mport, RIONET_MAILBOX);
	rio_release_outb_mbox(rnet->mport, RIONET_MAILBOX);
#endif

	return 0;
}

static void rionet_remove(struct rio_dev *rdev)
{
	struct net_device *ndev = NULL;
	struct rionet_peer *peer, *tmp;

	free_pages((unsigned long)rionet_active, rdev->net->hport->sys_size ?
					__ilog2(sizeof(void *)) + 4 : 0);
	unregister_netdev(ndev);
	kfree(ndev);

	list_for_each_entry_safe(peer, tmp, &rionet_peers, node) {
		list_del(&peer->node);
		kfree(peer);
	}
}

static void rionet_get_drvinfo(struct net_device *ndev,
			       struct ethtool_drvinfo *info)
{
	struct rionet_private *rnet = netdev_priv(ndev);

	strcpy(info->driver, DRV_NAME);
	strcpy(info->version, DRV_VERSION);
	strcpy(info->fw_version, "n/a");
	strcpy(info->bus_info, rnet->mport->name);
}

static u32 rionet_get_msglevel(struct net_device *ndev)
{
	struct rionet_private *rnet = netdev_priv(ndev);

	return rnet->msg_enable;
}

static void rionet_set_msglevel(struct net_device *ndev, u32 value)
{
	struct rionet_private *rnet = netdev_priv(ndev);

	rnet->msg_enable = value;
}

static const struct ethtool_ops rionet_ethtool_ops = {
	.get_drvinfo = rionet_get_drvinfo,
	.get_msglevel = rionet_get_msglevel,
	.set_msglevel = rionet_set_msglevel,
	.get_link = ethtool_op_get_link,
};

static const struct net_device_ops rionet_netdev_ops = {
	.ndo_open		= rionet_open,
	.ndo_stop		= rionet_close,
	.ndo_start_xmit		= rionet_start_xmit,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= eth_mac_addr,
};

static int rionet_setup_netdev(struct rio_mport *mport)
{
	int rc = 0;
	struct net_device *ndev = NULL;
	struct rionet_private *rnet;
	u16 device_id;

	/* Allocate our net_device structure */
	ndev = alloc_etherdev(sizeof(struct rionet_private));
	if (ndev == NULL) {
		printk(KERN_INFO "%s: could not allocate ethernet device.\n",
		       DRV_NAME);
		rc = -ENOMEM;
		goto out;
	}

	rionet_active = (struct rio_dev **)__get_free_pages(GFP_KERNEL,
			mport->sys_size ? __ilog2(sizeof(void *)) + 4 : 0);
	if (!rionet_active) {
		rc = -ENOMEM;
		goto out;
	}
	memset((void *)rionet_active, 0, sizeof(void *) *
				RIO_MAX_ROUTE_ENTRIES(mport->sys_size));

	/* Set up private area */
	rnet = netdev_priv(ndev);
	rnet->mport = mport;

	/* Set the default MAC address */
	device_id = rio_local_get_device_id(mport);
	ndev->dev_addr[0] = 0x00;
	ndev->dev_addr[1] = 0x01;
	ndev->dev_addr[2] = 0x00;
	ndev->dev_addr[3] = 0x01;
	ndev->dev_addr[4] = device_id >> 8;
	ndev->dev_addr[5] = device_id & 0xff;

	ndev->netdev_ops = &rionet_netdev_ops;
	ndev->mtu = RIO_MAX_MSG_SIZE - 14;
	ndev->features = NETIF_F_LLTX;
	SET_ETHTOOL_OPS(ndev, &rionet_ethtool_ops);

	spin_lock_init(&rnet->lock);
	spin_lock_init(&rnet->tx_lock);

	rnet->msg_enable = RIONET_DEFAULT_MSGLEVEL;

	rc = register_netdev(ndev);
	if (rc != 0)
		goto out;

	printk("%s: %s %s Version %s, MAC %pM\n",
	       ndev->name,
	       DRV_NAME,
	       DRV_DESC,
	       DRV_VERSION,
	       ndev->dev_addr);

      out:
	return rc;
}

/*
 * XXX Make multi-net safe
 */
static int rionet_probe(struct rio_dev *rdev, const struct rio_device_id *id)
{
	int rc = -ENODEV;
	u32 lpef, lsrc_ops, ldst_ops;
	struct rionet_peer *peer;

	/* If local device is not rionet capable, give up quickly */
	if (!rionet_capable)
		goto out;

	/*
	 * First time through, make sure local device is rionet
	 * capable, setup netdev,  and set flags so this is skipped
	 * on later probes
	 */
	if (!rionet_check) {
		rio_local_read_config_32(rdev->net->hport, RIO_PEF_CAR, &lpef);
		rio_local_read_config_32(rdev->net->hport, RIO_SRC_OPS_CAR,
					 &lsrc_ops);
		rio_local_read_config_32(rdev->net->hport, RIO_DST_OPS_CAR,
					 &ldst_ops);
		if (!is_rionet_capable(lpef, lsrc_ops, ldst_ops)) {
			printk(KERN_ERR
			       "%s: local device is not network capable\n",
			       DRV_NAME);
			rionet_check = 1;
			rionet_capable = 0;
			goto out;
		}

		rc = rionet_setup_netdev(rdev->net->hport);
		rionet_check = 1;
	}

	/*
	 * If the remote device has mailbox/doorbell capabilities,
	 * add it to the peer list.
	 */
	if (dev_rionet_capable(rdev)) {
		if (!(peer = kmalloc(sizeof(struct rionet_peer), GFP_KERNEL))) {
			rc = -ENOMEM;
			goto out;
		}
		peer->rdev = rdev;
		list_add_tail(&peer->node, &rionet_peers);
	}

      out:
	return rc;
}

static struct rio_device_id rionet_id_table[] = {
	{RIO_DEVICE(RIO_ANY_ID, RIO_ANY_ID)}
};

static struct rio_driver rionet_driver = {
	.name = "rionet",
	.id_table = rionet_id_table,
	.probe = rionet_probe,
	.remove = rionet_remove,
};

static int __init rionet_init(void)
{
	return rio_register_driver(&rionet_driver);
}

static void __exit rionet_exit(void)
{
	rio_unregister_driver(&rionet_driver);
}

module_init(rionet_init);
module_exit(rionet_exit);
