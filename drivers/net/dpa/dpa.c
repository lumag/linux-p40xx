/* Copyright (c) 2008 - 2009, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define CONFIG_DPA_RX_0_COPY 1
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sort.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/etherdevice.h>
#ifdef CONFIG_DPA_RX_0_COPY
#include <linux/if_arp.h>	/* arp_hdr_len() */
#include <linux/icmp.h>		/* struct icmphdr */
#include <linux/ip.h>		/* struct iphdr */
#include <linux/udp.h>		/* struct udphdr */
#include <linux/tcp.h>		/* struct tcphdr */
#include <linux/spinlock.h>
#include <linux/highmem.h>
#endif
#include <linux/percpu.h>

#include "linux/fsl_bman.h"

#include "fsl_fman.h"
#include "fm_ext.h"

#ifdef CONFIG_FSL_FMAN_TEST
#include "fsl_fman_test.h"
#endif /* CONFIG_FSL_FMAN_TEST */

#include "dpa.h"

#define ARRAY2_SIZE(arr)	(ARRAY_SIZE(arr) * ARRAY_SIZE((arr)[0]))

#define DPA_NETIF_FEATURES	0

#define DPA_DESCRIPTION "FSL DPA Ethernet driver"

MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Emil Medve <Emilian.Medve@Freescale.com>");

MODULE_DESCRIPTION(DPA_DESCRIPTION);

static uint8_t debug = -1;
module_param(debug, byte, S_IRUGO);
MODULE_PARM_DESC(debug, "Module/Driver verbosity level");

static uint16_t __devinitdata tx_timeout = 1000;
module_param(tx_timeout, ushort, S_IRUGO);
MODULE_PARM_DESC(tx_timeout, "The Tx timeout in ms");

static const char rtx[][3] = {
	[RX] = "RX",
	[TX] = "TX"
};

/* BM */

#ifdef DEBUG
#define GFP_DPA_BP	(GFP_DMA | __GFP_ZERO)
#define GFP_DPA		(GFP_KERNEL | __GFP_ZERO)
#else
#define GFP_DPA_BP	(GFP_DMA)
#define GFP_DPA		(GFP_KERNEL)
#endif

#define DPA_BP_HEAD	64
#define DPA_BP_SIZE(s)	(DPA_BP_HEAD + (s))

#define FM_FD_STAT_ERRORS							  \
	(FM_PORT_FRM_ERR_DMA			| FM_PORT_FRM_ERR_PHYSICAL	| \
	 FM_PORT_FRM_ERR_SIZE			| FM_PORT_FRM_ERR_CLS_DISCARD	| \
	 FM_PORT_FRM_ERR_EXTRACTION		| FM_PORT_FRM_ERR_NO_SCHEME	| \
	 FM_PORT_FRM_ERR_ILL_PLCR		| FM_PORT_FRM_ERR_PRS_TIMEOUT	| \
	 FM_PORT_FRM_ERR_PRS_ILL_INSTRUCT	| FM_PORT_FRM_ERR_PRS_HDR_ERR)

struct dpa_bp {
	struct bman_pool		*pool;
	union {
		struct list_head	list;
		uint8_t			bpid;
	};
	size_t				count;
	size_t				size;
	unsigned int			bp_kick_thresh;
	unsigned int			bp_blocks_per_page;
	char				kernel_pool;  /* kernel-owned pool */
	spinlock_t			lock;
	unsigned int			bp_refill_pending;
	union {
		struct page		**rxhash;
		struct {
			dma_addr_t	paddr;
			void *		vaddr;
		};
	};
};

static const size_t dpa_bp_size[] __devinitconst = {
	/* Keep these sorted */
	DPA_BP_SIZE(128), DPA_BP_SIZE(512), DPA_BP_SIZE(1536)
};

static unsigned int dpa_hash_rxaddr(const struct dpa_bp *bp, dma_addr_t a)
{
	a >>= PAGE_SHIFT;
	a ^= (a >> ilog2(bp->count));

	return (a % bp->count);
}

static struct page **dpa_find_rxpage(const struct dpa_bp *bp, dma_addr_t addr)
{
	unsigned int h = dpa_hash_rxaddr(bp, addr);
	int i, j;

	addr &= PAGE_MASK;
	for (i = h, j = 0; j < bp->count; j++, i = (i + 1) % bp->count)
		if (bp->rxhash[i] && bp->rxhash[i]->index == addr)
			return &bp->rxhash[i];

	return NULL;
}

static void dpa_hash_page(struct dpa_bp *bp, struct page *page, dma_addr_t base)
{
	unsigned int h = dpa_hash_rxaddr(bp, base);
	int i, j;

	page->index = base & PAGE_MASK;

	/* If the entry for this hash is missing, just find the next free one */
	for (i = h, j = 0; j < bp->count; j++, i = (i + 1) % bp->count) {
		if (bp->rxhash[i])
			continue;

		bp->rxhash[i] = page;
		return;
	}

	BUG();
}

static void bmb_free(const struct net_device *net_dev,
		     struct dpa_bp *bp, struct bm_buffer *bmb)
{
	int i;
	/*
	 * Go through the bmb array, and free/unmap every buffer remaining
	 * in it
	 */
	for (i = 0; i < 8; i++) {
		struct page **pageptr;
		struct page *page;
		unsigned long flags;

		if (!bmb[i].lo)
			break;

		spin_lock_irqsave(&bp->lock, flags);
		pageptr = dpa_find_rxpage(bp, bmb[i].lo);

		BUG_ON(!pageptr);

		page = *pageptr;
		*pageptr = NULL;

		spin_unlock_irqrestore(&bp->lock, flags);

		dma_unmap_page(net_dev->dev.parent, bmb[i].lo,
				bp->size, DMA_FROM_DEVICE);

		put_page(page);
	}
}

static void dpa_bp_refill(const struct net_device *net_dev, struct dpa_bp *bp,
			gfp_t mask)
{
	struct bm_buffer bmb[8];
	int err;
	unsigned int blocks;
	unsigned int blocks_per_page = bp->bp_blocks_per_page;
	unsigned int block_size = bp->size;
	struct page *page = NULL;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&bp->lock, flags);
	/* Round down to an integral number of pages */
	blocks = (bp->bp_refill_pending / blocks_per_page) * blocks_per_page;

	bp->bp_refill_pending -= blocks;

	spin_unlock_irqrestore(&bp->lock, flags);

	for (i = 0; i < blocks; i++) {
		dma_addr_t addr;
		unsigned int off = (i % blocks_per_page) * block_size;

		/* Do page allocation every time we need a new page */
		if ((i % blocks_per_page) == 0) {
			page = alloc_page(mask);

			if (!page)
				break;

			/* Update the reference count to reflect # of buffers */
			if (blocks_per_page > 1)
				atomic_add(blocks_per_page - 1,
						&compound_head(page)->_count);
		}

		addr = dma_map_page(net_dev->dev.parent, page, off, block_size,
					DMA_FROM_DEVICE);

		spin_lock_irqsave(&bp->lock, flags);
		/* Add the page to the hash table */
		dpa_hash_page(bp, page, addr);

		spin_unlock_irqrestore(&bp->lock, flags);

		/*
		 * Fill the release buffer to release as many at a time as
		 * is possible.
		 */
		bmb[i % 8].hi = 0;
		bmb[i % 8].lo = addr;

		/* When we get to the end of the buffer, send it all to bman */
		if ((i % 8) == 7) {
			err = bman_release(bp->pool, bmb, 8,
						BMAN_RELEASE_FLAG_WAIT_INT);

			if (err < 0) {
				bmb_free(net_dev, bp, bmb);
				return;
			}
		}
	}

	/* Take care of the leftovers ('i' will be one past the last block) */
	if ((i % 8) != 0) {
		err = bman_release(bp->pool, bmb, i % 8,
				BMAN_RELEASE_FLAG_WAIT_INT);

		if (err < 0) {
			int j;

			for (j = i % 8; j < 8; j++)
				bmb[j].lo = 0;
			bmb_free(net_dev, bp, bmb);
			return;
		}
	}
}

static void __cold dpa_bp_depletion(struct bman_portal	*portal,
				    struct bman_pool	*pool,
				    void		*cb_ctx,
				    int			 depleted)
{
	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	BUG();

	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);
}

static int __devinit __must_check __cold __attribute__((nonnull))
_dpa_bp_alloc(struct net_device *net_dev, struct list_head *list,
		struct dpa_bp *dpa_bp)
{
	int			 _errno;
	struct bman_pool_params	 bp_params;

	BUG_ON(dpa_bp->size == 0);
	BUG_ON(dpa_bp->count == 0);

	bp_params.flags		= BMAN_POOL_FLAG_DEPLETION;
	bp_params.cb		= dpa_bp_depletion;
	bp_params.cb_ctx	= dpa_bp;

	if (dpa_bp->bpid == 0)
		bp_params.flags |= BMAN_POOL_FLAG_DYNAMIC_BPID;
	else
		bp_params.bpid = dpa_bp->bpid;

	dpa_bp->pool = bman_new_pool(&bp_params);
	if (unlikely(dpa_bp->pool == NULL)) {
		cpu_dev_err(net_dev->dev.parent,
				"%s:%hu:%s(): bman_new_pool() failed\n",
				__file__, __LINE__, __func__);
		_errno = -ENOMEM;
		goto _return_bm_pool_free;
	}

	/* paddr is only set for pools that are shared between partitions */
	if (dpa_bp->paddr == 0) {
		dpa_bp->rxhash = kzalloc(dpa_bp->count * sizeof(struct page *),
					 GFP_KERNEL);
		if (!dpa_bp->rxhash) {
			_errno = -ENOMEM;
			goto _return_bman_free_pool;
		}

		dpa_bp->bp_blocks_per_page = PAGE_SIZE / dpa_bp->size;
		BUG_ON(dpa_bp->count < dpa_bp->bp_blocks_per_page);
		dpa_bp->bp_kick_thresh = dpa_bp->count - max((unsigned int)8,
						dpa_bp->bp_blocks_per_page);
		dpa_bp->bp_refill_pending = dpa_bp->count;
		dpa_bp->kernel_pool = 1;

		spin_lock_init(&dpa_bp->lock);
		dpa_bp_refill(net_dev, dpa_bp, GFP_KERNEL);
	} else {
		/* This is a shared pool, which the kernel doesn't manage */
		dpa_bp->kernel_pool = 0;
		devm_request_mem_region(net_dev->dev.parent, dpa_bp->paddr,
					dpa_bp->size * dpa_bp->count,
					KBUILD_MODNAME);
		dpa_bp->vaddr = devm_ioremap_prot(net_dev->dev.parent,
					dpa_bp->paddr,
						  dpa_bp->size * dpa_bp->count,
						  0);
		if (unlikely(dpa_bp->vaddr == NULL)) {
			cpu_dev_err(net_dev->dev.parent,
					"%s:%hu:%s(): devm_ioremap() failed\n",
					__file__, __LINE__, __func__);
			_errno = -EIO;
			goto _return_bman_free_pool;
		}
	}

	list_add_tail(&dpa_bp->list, list);

	return 0;

_return_bman_free_pool:
	bman_free_pool(dpa_bp->pool);
_return_bm_pool_free:
	if (dpa_bp->bpid == 0)
		bm_pool_free(bp_params.bpid);

	return _errno;
}

static struct dpa_bp * __must_check __attribute__((nonnull))
dpa_size2pool(const struct list_head *list, size_t size)
{
	struct dpa_bp	*dpa_bp;

	list_for_each_entry(dpa_bp, list, list) {
		if (DPA_BP_SIZE(size) <= dpa_bp->size)
			return dpa_bp;
	}
	return ERR_PTR(-ENODEV);
}

static struct dpa_bp * __must_check __attribute__((nonnull))
dpa_bpid2pool(const struct list_head *list, int bpid)
{
	struct dpa_bp	*dpa_bp;

	list_for_each_entry(dpa_bp, list, list) {
		if (bman_get_params(dpa_bp->pool)->bpid == bpid)
			return dpa_bp;
	}
	return ERR_PTR(-EINVAL);
}

static int __cold __must_check __attribute__((nonnull))
dpa_pool2bpid(const struct dpa_bp *dpa_bp)
{
	return bman_get_params(dpa_bp->pool)->bpid;
}

static void * __must_check __attribute__((nonnull))
dpa_phys2virt(const struct dpa_bp *dpa_bp, const struct bm_buffer *bmb)
{
#ifdef CONFIG_BUG
	const struct bman_pool_params	*bp_params;

	bp_params = bman_get_params(dpa_bp->pool);
#endif
	BUG_ON(bp_params->cb_ctx != dpa_bp);
	BUG_ON(bp_params->bpid != bmb->bpid);

	return dpa_bp->vaddr + (bmb->lo - dpa_bp->paddr);
}

static void __cold __attribute__((nonnull))
_dpa_bp_free(struct device *dev, struct dpa_bp *dpa_bp)
{
	uint8_t	bpid;

	if (dpa_bp->kernel_pool) {
		kfree(dpa_bp->rxhash);
	} else {
		dma_unmap_single(dev, dpa_bp->paddr,
			dpa_bp->size * dpa_bp->count, DMA_BIDIRECTIONAL);
		free_pages_exact(dpa_bp->vaddr, dpa_bp->size * dpa_bp->count);
	}
	bpid = dpa_pool2bpid(dpa_bp);
	bman_free_pool(dpa_bp->pool);
	list_del(&dpa_bp->list);
	bm_pool_free(bpid);
}

static void __cold __attribute__((nonnull))
dpa_bp_free(struct device *dev, struct list_head *list)
{
	struct dpa_bp	*dpa_bp, *tmp;

	list_for_each_entry_safe(dpa_bp, tmp, list, list)
		_dpa_bp_free(dev, dpa_bp);
}

/* QM */

struct dpa_fq {
	struct qman_fq	 	 fq_base;
	struct list_head	 list;
	struct net_device	*net_dev;
	bool			 init;
};

static struct qman_fq * __devinit __cold __must_check __attribute__((nonnull))
_dpa_fq_alloc(struct list_head		*list,
	      struct dpa_fq		*dpa_fq,
	      uint32_t			 fqid,
	      uint32_t			 flags,
	      uint16_t			 channel,
	      uint8_t			 wq)
{
	int			 _errno;
	const struct dpa_priv_s	*priv;
	struct device		*dev;
	struct qman_fq		*fq;
	struct qm_mcc_initfq	 initfq;

	priv = (typeof(priv))netdev_priv(dpa_fq->net_dev);
	dev = dpa_fq->net_dev->dev.parent;

	if (fqid == 0) {
		flags |= QMAN_FQ_FLAG_DYNAMIC_FQID;
		flags &= ~QMAN_FQ_FLAG_NO_MODIFY;
	} else {
		flags &= ~QMAN_FQ_FLAG_DYNAMIC_FQID;
	}

	dpa_fq->init	= !(flags & QMAN_FQ_FLAG_NO_MODIFY);

	_errno = qman_create_fq(fqid, flags, &dpa_fq->fq_base);
	if (unlikely(_errno)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev,
				"%s:%hu:%s(): qman_create_fq() failed\n",
				__file__, __LINE__, __func__);
		return ERR_PTR(_errno);
	}
	fq = &dpa_fq->fq_base;

	if (dpa_fq->init) {
		initfq.we_mask		= QM_INITFQ_WE_DESTWQ;
		initfq.fqd.dest.channel	= channel;
		initfq.fqd.dest.wq	= wq;

		_errno = qman_init_fq(fq, QMAN_INITFQ_FLAG_SCHED, &initfq);
		if (unlikely(_errno < 0)) {
			if (netif_msg_probe(priv))
				cpu_dev_err(dev,
					"%s:%hu:%s(): qman_init_fq(%u) = %d\n",
					__file__, __LINE__, __func__,
					qman_fq_fqid(fq), _errno);
			qman_destroy_fq(fq, 0);
			return ERR_PTR(_errno);
		}
	}

	list_add_tail(&dpa_fq->list, list);

	return fq;
}

static int __cold __attribute__((nonnull))
_dpa_fq_free(struct device *dev, struct qman_fq *fq)
{
	int		 _errno, __errno;
	struct dpa_fq	*dpa_fq;

	_errno = 0;

	dpa_fq = (struct dpa_fq *)fq;
	if (dpa_fq->init) {
		_errno = qman_retire_fq(fq, NULL);
		if (unlikely(_errno < 0))
			cpu_dev_err(dev,
				"%s:%hu:%s(): qman_retire_fq(%u) = %d\n",
				__file__, __LINE__, __func__, qman_fq_fqid(fq),
				_errno);

		__errno = qman_oos_fq(fq);
		if (unlikely(__errno < 0)) {
			cpu_dev_err(dev, "%s:%hu:%s(): qman_oos_fq(%u) = %d\n",
					__file__, __LINE__, __func__,
					qman_fq_fqid(fq), __errno);
			if (_errno >= 0)
				_errno = __errno;
		}
	}

	qman_destroy_fq(fq, 0);
	list_del(&dpa_fq->list);

	return _errno;
}

static int __cold __attribute__((nonnull))
dpa_fq_free(struct device *dev, struct list_head *list)
{
	int		 _errno, __errno;
	struct dpa_fq	*dpa_fq, *tmp;

	_errno = 0;
	list_for_each_entry_safe(dpa_fq, tmp, list, list) {
		__errno = _dpa_fq_free(dev, (struct qman_fq *)dpa_fq);
		if (unlikely(__errno < 0) && _errno >= 0)
			_errno = __errno;
	}

	return _errno;
}

struct dpa_fd {
	struct qm_fd		 fd;
	struct list_head	 list;
};

static inline ssize_t __const __must_check __attribute__((nonnull))
dpa_fd_length(const struct qm_fd *fd)
{
	switch (fd->format) {
	case qm_fd_contig:
	case qm_fd_sg:
		return fd->length20;
	case qm_fd_contig_big:
	case qm_fd_sg_big:
		return fd->length29;
	case qm_fd_compound:
		return fd->cong_weight;
	}
	BUG();
	return -EINVAL;
}

static inline ssize_t __const __must_check __attribute__((nonnull))
dpa_fd_offset(const struct qm_fd *fd)
{
	switch (fd->format) {
	case qm_fd_contig:
	case qm_fd_sg:
		return fd->offset;
	case qm_fd_contig_big:
	case qm_fd_sg_big:
	case qm_fd_compound:
		return 0;
	}
	BUG();
	return -EINVAL;
}

static int __must_check __attribute__((nonnull))
dpa_fd_release(const struct net_device *net_dev, const struct qm_fd *fd)
{
	int				 _errno, __errno, i, j;
	const struct dpa_priv_s		*priv;
	const struct qm_sg_entry	*sgt;
	struct dpa_bp		*_dpa_bp, *dpa_bp;
	const struct bm_buffer		*_bmb;
	struct bm_buffer		 bmb[8];

	priv = (typeof(priv))netdev_priv(net_dev);

	_bmb = (typeof(_bmb))fd;

	_dpa_bp = dpa_bpid2pool(&priv->dpa_bp_list, _bmb->bpid);
	BUG_ON(IS_ERR(_dpa_bp));

	_errno = 0;
	if (fd->format == qm_fd_sg) {
#ifdef CONFIG_DPA_RX_0_COPY
		struct page **pageptr;
		struct page *page = NULL;

		if (_dpa_bp->kernel_pool) {
			unsigned long flags;
			spin_lock_irqsave(&_dpa_bp->lock, flags);
			pageptr = dpa_find_rxpage(_dpa_bp, _bmb->lo);
			page = *pageptr;
			spin_unlock_irqrestore(&_dpa_bp->lock, flags);

			sgt = (typeof(sgt))(kmap(page) +
				(_bmb->lo & ~PAGE_MASK) + dpa_fd_offset(fd));
		} else
#endif
		{
			sgt = (typeof(sgt))(dpa_phys2virt(_dpa_bp, _bmb)
				+ dpa_fd_offset(fd));
		}

		i = 0;
		do {
			dpa_bp = dpa_bpid2pool(&priv->dpa_bp_list, sgt[i].bpid);
			BUG_ON(IS_ERR(dpa_bp));

			j = 0;
			do {
				BUG_ON(sgt[i].extension);

				bmb[j].bpid	= sgt[i].bpid;
				bmb[j].hi	= sgt[i].addr_hi;
				bmb[j].lo	= sgt[i].addr_lo;
				j++; i++;
			} while (j < ARRAY_SIZE(bmb) &&
					!sgt[i-1].final &&
					sgt[i-1].bpid == sgt[i].bpid);

			__errno = bman_release(dpa_bp->pool, bmb, j,
					BMAN_RELEASE_FLAG_WAIT_INT);
			if (unlikely(__errno < 0)) {
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): bman_release(%hu) = %d\n",
					__file__, __LINE__, __func__,
					bman_get_params(dpa_bp->pool)->bpid,
					_errno);
				if (_errno >= 0)
					_errno = __errno;
			}
		} while (!sgt[i-1].final);

#ifdef CONFIG_DPA_RX_0_COPY
		if (_dpa_bp->kernel_pool)
			kunmap(page);
#endif
	}

	__errno = bman_release(_dpa_bp->pool, _bmb, 1,
			BMAN_RELEASE_FLAG_WAIT_INT);
	if (unlikely(__errno < 0)) {
		cpu_netdev_err(net_dev, "%s:%hu:%s(): bman_release(%hu) = %d\n",
				__file__, __LINE__, __func__,
				bman_get_params(_dpa_bp->pool)->bpid, _errno);
		if (_errno >= 0)
			_errno = __errno;
	}

	return _errno;
}

/* net_device */

#ifdef CONFIG_DPA_RX_0_COPY
#define NN_ALLOCATED_SPACE(net_dev)	max((size_t)arp_hdr_len(net_dev),  sizeof(struct iphdr))
#define NN_RESERVED_SPACE(net_dev)	min((size_t)arp_hdr_len(net_dev),  sizeof(struct iphdr))

#define TT_ALLOCATED_SPACE(net_dev)	\
	max(sizeof(struct icmphdr), max(sizeof(struct udphdr), sizeof(struct tcphdr)))
#define TT_RESERVED_SPACE(net_dev)	\
	min(sizeof(struct icmphdr), min(sizeof(struct udphdr), sizeof(struct tcphdr)))
#endif

static enum qman_cb_dqrr_result
ingress_rx_error_dqrr(struct qman_portal *portal, struct qman_fq *fq,
		const struct qm_dqrr_entry *dq)
{
	int			 _errno;
	struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	net_dev->stats.rx_errors++;
	net_dev->stats.rx_packets++;
	net_dev->stats.rx_bytes += dpa_fd_length(&dq->fd);

	BUG_ON((dq->fd.status & FM_FD_STAT_ERRORS) == 0);

	cpu_netdev_err(net_dev, "%s:%hu:%s(): FD status = 0x%08x\n",
			__file__, __LINE__, __func__,
			dq->fd.status & FM_FD_STAT_ERRORS);

#ifdef CONFIG_FSL_FMAN_TEST
{
    const struct bm_buffer *bmb;
    const struct dpa_bp *dpa_bp;

    bmb = (typeof(bmb))&dq->fd;

    dpa_bp = dpa_bpid2pool(&priv->dpa_bp_list, bmb->bpid);
    BUG_ON(IS_ERR(dpa_bp));

	is_fman_test(priv->mac_dev, FMT_RX_ERR_Q, dpa_phys2virt(dpa_bp, bmb),
		     dq->fd.length20 + dq->fd.offset);
}
#endif	/* CONFIG_FSL_FMAN_TEST */

        _errno = dpa_fd_release(net_dev, &dq->fd);
        if (unlikely(_errno < 0)) {
            dump_stack();
            panic("Can't release buffer to the BM during RX\n");
        }

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return qman_cb_dqrr_consume;
}

static void ingress_rx_error_ern(struct qman_portal		*portal,
				 struct qman_fq			*fq,
				 const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_rx_error_dc_ern(struct qman_portal		*portal,
				    struct qman_fq		*fq,
				    const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_rx_error_fqs(struct qman_portal		*portal,
				 struct qman_fq			*fq,
				 const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static enum qman_cb_dqrr_result __hot
ingress_rx_default_dqrr(struct qman_portal		*portal,
			struct qman_fq			*fq,
			const struct qm_dqrr_entry	*dq)
{
	const struct net_device	*net_dev;
	struct dpa_priv_s	*priv;
	struct dpa_fd		*dpa_fd;
	struct fd_list_head	*fd_list;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_intr(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

#ifdef CONFIG_FSL_FMAN_TEST
{
    const struct bm_buffer	*bmb;
    const struct dpa_bp		*dpa_bp;
    void   					*virt;
    int						 _errno, i;
	struct dpa_fq			*dpa_fq;
	uint32_t				fqid=0;

	for(i=0; i<priv->num; i++) {
		if((dq->fqid >= priv->ranges[i].base) &&
		   (dq->fqid < (priv->ranges[i].base+priv->ranges[i].count))) {
			fqid = dq->fqid - priv->ranges[i].base;
			break;
		}
	}

	if (i == priv->num)
		fqid = FMT_RX_DFLT_Q;

	if (fqid == FMT_RX_DFLT_Q) {
		list_for_each_entry(dpa_fq, priv->dpa_fq_list + RX, list) {
			if (dq->fqid ==
				qman_fq_fqid((struct qman_fq *)dpa_fq)) {
				fqid = FMT_RX_ERR_Q;
				break;
			}
		}
		if (fqid == FMT_RX_ERR_Q)
			fqid = FMT_RX_DFLT_Q;
		else
			BUG();
	}

    bmb = (typeof(bmb))&dq->fd;

    dpa_bp = dpa_bpid2pool(&priv->dpa_bp_list, bmb->bpid);
    BUG_ON(IS_ERR(dpa_bp));

    virt = dpa_phys2virt(dpa_bp, bmb);
    if (is_fman_test((void *)priv->mac_dev,
                     fqid,
                     virt,
                     dq->fd.length20 + dq->fd.offset)) {
        _errno = dpa_fd_release(net_dev, &dq->fd);
        if (unlikely(_errno < 0)) {
            dump_stack();
            panic("Can't release buffer to the BM during RX\n");
        }
        goto _return;
    }
}
#endif /* CONFIG_FSL_FMAN_TEST */

	dpa_fd = (typeof(dpa_fd))devm_kzalloc(net_dev->dev.parent,
			sizeof(*dpa_fd), GFP_ATOMIC);
	if (unlikely(dpa_fd == NULL)) {
		if (netif_msg_rx_err(priv))
			cpu_netdev_err(net_dev,
				"%s:%hu:%s(): devm_kzalloc() failed\n",
				__file__, __LINE__, __func__);
		goto _return;
	}

	dpa_fd->fd = dq->fd;

	fd_list = per_cpu_ptr(priv->fd_list, smp_processor_id());
	list_add_tail(&dpa_fd->list, &fd_list->list);
	fd_list->count++;
	fd_list->max = max(fd_list->max, fd_list->count);

	schedule_work(&priv->fd_work);

_return:
	if (netif_msg_intr(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return qman_cb_dqrr_consume;
}

static void ingress_rx_default_ern(struct qman_portal		*portal,
				   struct qman_fq		*fq,
				   const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_rx_default_dc_ern(struct qman_portal	*portal,
			      struct qman_fq		*fq,
			      const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_rx_default_fqs(struct qman_portal		*portal,
				   struct qman_fq		*fq,
				   const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static enum qman_cb_dqrr_result
ingress_tx_error_dqrr(struct qman_portal *portal, struct qman_fq *fq,
		const struct qm_dqrr_entry *dq)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

#ifdef CONFIG_FSL_FMAN_TEST
{
    void   *virt = bus_to_virt(dq->fd.addr_lo);
    if (is_fman_test((void *)priv->mac_dev,
                     FMT_TX_ERR_Q,
                     virt,
                     dq->fd.length20 + dq->fd.offset)) {
        return qman_cb_dqrr_consume;
    }
}
#endif /* CONFIG_FSL_FMAN_TEST */

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return qman_cb_dqrr_consume;
}

static void ingress_tx_error_ern(struct qman_portal		*portal,
				 struct qman_fq			*fq,
				 const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_tx_error_dc_ern(struct qman_portal		*portal,
				    struct qman_fq		*fq,
				    const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_tx_error_fqs(struct qman_portal		*portal,
				 struct qman_fq			*fq,
				 const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static enum qman_cb_dqrr_result
ingress_tx_default_dqrr(struct qman_portal		*portal,
			struct qman_fq			*fq,
			const struct qm_dqrr_entry	*dq)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;
	struct sk_buff		*skb;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

#ifdef CONFIG_FSL_FMAN_TEST
{
    void   *virt = bus_to_virt(dq->fd.addr_lo);
    if (is_fman_test((void *)priv->mac_dev,
                     FMT_TX_CONF_Q,
                     virt,
                     dq->fd.length20 + dq->fd.offset)) {
        return qman_cb_dqrr_consume;
    }
}
#endif /* CONFIG_FSL_FMAN_TEST */

	skb = *(typeof(&skb))bus_to_virt(dq->fd.addr_lo);

	BUG_ON(net_dev != skb->dev);

	dma_unmap_single(net_dev->dev.parent, dq->fd.addr_lo, skb_headlen(skb),
			DMA_TO_DEVICE);

	dev_kfree_skb_irq(skb);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return qman_cb_dqrr_consume;
}

static void ingress_tx_default_ern(struct qman_portal		*portal,
				   struct qman_fq		*fq,
				   const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_tx_default_dc_ern(struct qman_portal	*portal,
				      struct qman_fq		*fq,
				      const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void ingress_tx_default_fqs(struct qman_portal		*portal,
				   struct qman_fq		*fq,
				   const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static enum qman_cb_dqrr_result egress_dqrr(struct qman_portal		*portal,
					    struct qman_fq		*fq,
					    const struct qm_dqrr_entry	*dq)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return qman_cb_dqrr_consume;
}

static void egress_ern(struct qman_portal	*portal,
		       struct qman_fq		*fq,
		       const struct qm_mr_entry	*msg)
{
	int			 _errno;
	struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;
	struct sk_buff		*skb;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	if (priv->mac_dev) {
		skb = *(typeof(&skb))bus_to_virt(msg->ern.fd.addr_lo);

		BUG_ON(net_dev != skb->dev);

		dma_unmap_single(net_dev->dev.parent, msg->ern.fd.addr_lo,
				skb_headlen(skb), DMA_TO_DEVICE);

		dev_kfree_skb_irq(skb);
	} else {
		_errno = dpa_fd_release(net_dev, &msg->ern.fd);
		if (unlikely(_errno < 0)) {
			dump_stack();
			panic("Can't release buffer to the BM during a TX\n");
		}
	}

	net_dev->stats.tx_dropped++;
	net_dev->stats.tx_fifo_errors++;

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void egress_dc_ern(struct qman_portal		*portal,
			  struct qman_fq		*fq,
			  const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void egress_fqs(struct qman_portal	*portal,
		       struct qman_fq		*fq,
		       const struct qm_mr_entry	*msg)
{
	const struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;

	net_dev = ((struct dpa_fq *)fq)->net_dev;
	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	BUG();

	if (netif_msg_tx_err(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static const struct qman_fq ingress_fqs[][2] __devinitconst = {
	[RX] = {
		/* Error */
		{.cb = {ingress_rx_error_dqrr, ingress_rx_error_ern,
			ingress_rx_error_dc_ern, ingress_rx_error_fqs} },
		 /* Default */
		{.cb = {ingress_rx_default_dqrr, ingress_rx_default_ern,
			ingress_rx_default_dc_ern, ingress_rx_default_fqs} }
	},
	[TX] = {
		/* Error */
		{.cb = {ingress_tx_error_dqrr, ingress_tx_error_ern,
			ingress_tx_error_dc_ern, ingress_tx_error_fqs} },
		 /* Default */
		{.cb = {ingress_tx_default_dqrr, ingress_tx_default_ern,
			ingress_tx_default_dc_ern, ingress_tx_default_fqs} }
	}
};

static const struct qman_fq _egress_fqs __devinitconst = {
	.cb = {egress_dqrr, egress_ern, egress_dc_ern, egress_fqs}
};

static struct net_device_stats * __cold
dpa_get_stats(struct net_device *net_dev)
{
	cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return &net_dev->stats;
}

static void __cold dpa_change_rx_flags(struct net_device *net_dev, int flags)
{
	int			 _errno;
	const struct dpa_priv_s	*priv;

	cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	priv = (typeof(priv))netdev_priv(net_dev);

	if ((flags & IFF_PROMISC) != 0 && priv->mac_dev != NULL) {
		_errno = priv->mac_dev->change_promisc(priv->mac_dev);
		if (unlikely(_errno < 0))
			cpu_netdev_err(net_dev,
				"%s:%hu:%s(): mac_dev->change_promisc() = %d\n",
				__file__, __LINE__, __func__, _errno);
	}

	cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static void __hot dpa_rx(struct work_struct *fd_work)
{
	int			 _errno;
	struct net_device	*net_dev;
	const struct dpa_priv_s	*priv;
	struct fd_list_head	*fd_list;
	struct dpa_fd		*dpa_fd, *tmp;
	const struct bm_buffer	*bmb;
	struct dpa_bp	*dpa_bp;
	size_t			 head, size;
	struct sk_buff		*skb;
#ifdef CONFIG_DPA_RX_0_COPY
	struct page **pageptr;
	struct page *page = NULL;
	unsigned long flags;
#endif

	priv = (typeof(priv))container_of(fd_work, struct dpa_priv_s, fd_work);
	net_dev = priv->net_dev;

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	fd_list = per_cpu_ptr(priv->fd_list, smp_processor_id());
	list_for_each_entry_safe(dpa_fd, tmp, &fd_list->list, list) {
		skb = NULL;

		if (unlikely(dpa_fd->fd.status & FM_FD_STAT_ERRORS) != 0) {
			if (netif_msg_rx_err(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): FD status = 0x%08x\n",
					__file__, __LINE__, __func__,
					dpa_fd->fd.status & FM_FD_STAT_ERRORS);

			net_dev->stats.rx_errors++;

			goto _continue_dpa_fd_release;
		}

		net_dev->stats.rx_packets++;
		net_dev->stats.rx_bytes += dpa_fd_length(&dpa_fd->fd);

		if (dpa_fd->fd.format == qm_fd_sg) {
			net_dev->stats.rx_dropped++;
			printk("dropping an SG frame");

			goto _continue_dpa_fd_release;
		}

		BUG_ON(dpa_fd->fd.format != qm_fd_contig);

		bmb = (typeof(bmb))&dpa_fd->fd;

		dpa_bp = dpa_bpid2pool(&priv->dpa_bp_list, bmb->bpid);
		BUG_ON(IS_ERR(dpa_bp));

#ifdef CONFIG_DPA_RX_0_COPY
		if (dpa_bp->kernel_pool) {
			spin_lock_irqsave(&dpa_bp->lock, flags);
			pageptr = dpa_find_rxpage(dpa_bp, bmb->lo);

			if (!pageptr)
				cpu_pr_emerg("No page found for addr %x!\n",
						bmb->lo);

			page = *pageptr;
			*pageptr = NULL;

			dpa_bp->bp_refill_pending++;
			spin_unlock_irqrestore(&dpa_bp->lock, flags);

			head = sizeof(*bmb) + NET_IP_ALIGN;
			size = ETH_HLEN + NN_ALLOCATED_SPACE(net_dev) +
				TT_ALLOCATED_SPACE(net_dev);
		} else
#endif
		{
			head = NET_IP_ALIGN;
			size = dpa_fd_length(&dpa_fd->fd);
		}

		skb = __netdev_alloc_skb(net_dev, head + size, GFP_DPA);
		if (unlikely(skb == NULL)) {
			if (netif_msg_rx_err(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu: netdev_alloc_skb failed\n",
					__file__, __LINE__);

			net_dev->stats.rx_dropped++;

			goto _continue_dpa_fd_release;
		}

		skb_reserve(skb, head);

#ifdef CONFIG_DPA_RX_0_COPY
		if (dpa_bp->kernel_pool) {
			unsigned int off = (bmb->lo +
				dpa_fd_offset(&dpa_fd->fd)) & ~PAGE_MASK;
			unsigned int len = dpa_fd_length(&dpa_fd->fd);
			*(struct bm_buffer *)skb->head = *bmb;

			skb_fill_page_desc(skb, 0, page, off, len);

			skb->len	+= len;
			skb->data_len	+= len;
			skb->truesize	+= dpa_bp->size;

			/*
			 * Unmap this page mapping,
			 * and remove one instance of the page from the hash
			 */
			dma_unmap_page(net_dev->dev.parent, page->index + off,
					dpa_bp->size, DMA_FROM_DEVICE);

			if (unlikely(!__pskb_pull_tail(skb,
						ETH_HLEN +
						NN_RESERVED_SPACE(net_dev) +
						TT_RESERVED_SPACE(net_dev)))) {
				if (netif_msg_rx_err(priv))
					cpu_netdev_err(net_dev,
					"%s:%hu: __pskb_pull_tail() failed\n",
						       __file__, __LINE__);

				net_dev->stats.rx_dropped++;

				goto _continue_dev_kfree_skb;
			}
		} else
#endif
		{
			memcpy(skb_put(skb, size),
				dpa_phys2virt(dpa_bp, bmb) +
					dpa_fd_offset(&dpa_fd->fd),
				size);

			_errno = dpa_fd_release(net_dev, &dpa_fd->fd);
			if (unlikely(_errno < 0)) {
				dump_stack();
				panic("Can't release buffer to BM during RX\n");
			}
		}

		skb->protocol = eth_type_trans(skb, net_dev);
#if defined(CONFIG_FSL_FMAN_TEST) || defined(CONFIG_FSL_FMAN_TEST_LOOP)
{
    struct iphdr    *iph = (struct iphdr *)(skb->data);
    uint32_t        net;
    uint32_t        saddr = iph->saddr;
    uint32_t        daddr = iph->daddr;

    /* If it is ARP packet ... */
    if (*(uint32_t*)skb->data == 0x00010800)
    {
        saddr = *((uint32_t*)(skb->data+14));
        daddr = *((uint32_t*)(skb->data+24));
    }

    cpu_dev_dbg (net_dev->dev.parent,
                 "Src  IP before header-manipulation: %d.%d.%d.%d\n",
                 (int)((saddr & 0xff000000) >> 24),
                 (int)((saddr & 0x00ff0000) >> 16),
                 (int)((saddr & 0x0000ff00) >> 8),
                 (int)((saddr & 0x000000ff) >> 0));
    cpu_dev_dbg (net_dev->dev.parent,
                 "Dest IP before header-manipulation: %d.%d.%d.%d\n",
                 (int)((daddr & 0xff000000) >> 24),
                 (int)((daddr & 0x00ff0000) >> 16),
                 (int)((daddr & 0x0000ff00) >> 8),
                 (int)((daddr & 0x000000ff) >> 0));

    /* We allow only up to 10 eth ports */
#if defined(CONFIG_FSL_FMAN_TEST)
    net   = ((daddr & 0x000000ff) % 10);
    saddr = (uint32_t)((saddr & ~0x0000ff00) | (net << 8));
    daddr = (uint32_t)((daddr & ~0x0000ff00) | (net << 8));
#else /* loopback */
    net   = saddr;
    saddr = daddr;
    daddr = net;
#endif /* defined(CONFIG_FSL_FMAN_TEST) */

    /* If not ARP ... */
    if (*(uint32_t*)skb->data != 0x00010800)
    {
        iph->check = 0;

        iph->saddr = saddr;
        iph->daddr = daddr;
        iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
    }
    else /* The packet is ARP */
    {
        *(uint32_t*)(skb->data+14) = saddr;
        *(uint32_t*)(skb->data+24) = daddr;
    }

    cpu_dev_dbg (net_dev->dev.parent,
                 "Src  IP after  header-manipulation: %d.%d.%d.%d\n",
                 (int)((saddr & 0xff000000) >> 24),
                 (int)((saddr & 0x00ff0000) >> 16),
                 (int)((saddr & 0x0000ff00) >> 8),
                 (int)((saddr & 0x000000ff) >> 0));
    cpu_dev_dbg (net_dev->dev.parent,
                 "Dest IP after  header-manipulation: %d.%d.%d.%d\n",
                 (int)((daddr & 0xff000000) >> 24),
                 (int)((daddr & 0x00ff0000) >> 16),
                 (int)((daddr & 0x0000ff00) >> 8),
                 (int)((daddr & 0x000000ff) >> 0));
}
#endif /* defined(CONFIG_FSL_FMAN_TEST) */

		_errno = netif_rx_ni(skb);
		if (unlikely(_errno != NET_RX_SUCCESS)) {
			if (netif_msg_rx_status(priv))
				cpu_netdev_warn(net_dev, "%s:%hu:%s(): netif_rx_ni() = %d\n",
						__file__, __LINE__, __func__, _errno);
			net_dev->stats.rx_dropped++;
		}

		net_dev->last_rx = jiffies;

#ifdef CONFIG_DPA_RX_0_COPY
		if (dpa_bp->bp_refill_pending > dpa_bp->bp_kick_thresh)
			dpa_bp_refill(net_dev, dpa_bp, GFP_KERNEL);
#endif

		goto _continue;

_continue_dpa_fd_release:
		_errno = dpa_fd_release(net_dev, &dpa_fd->fd);
		if (unlikely(_errno < 0)) {
			dump_stack();
			panic("Can't release buffer to the BM during RX\n");
		}
#ifdef CONFIG_DPA_RX_0_COPY
_continue_dev_kfree_skb:
		dev_kfree_skb(skb);
#endif
_continue:

		local_irq_disable();
		list_del(&dpa_fd->list);
		fd_list->count--;
		local_irq_enable();

		devm_kfree(net_dev->dev.parent, dpa_fd);
	}

	if (netif_msg_rx_status(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static int __hot dpa_tx(struct sk_buff *skb, struct net_device *net_dev)
{
	int			 _errno, __errno;
	const struct dpa_priv_s	*priv;
	struct device		*dev;
	struct qm_fd		 fd;
	struct dpa_bp		*dpa_bp = NULL;
	struct bm_buffer	*bmb = NULL;

	priv = (typeof(priv))netdev_priv(net_dev);
	dev = net_dev->dev.parent;

	if (netif_msg_tx_queued(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	/* We don't have yet support for SG */
	SKB_LINEAR_ASSERT(skb);

	memset(&fd, 0, sizeof(fd));
	fd.format	= qm_fd_contig;
	fd.offset	= DPA_BP_HEAD;

	if (priv->mac_dev) {
		*((typeof(&skb))skb_push(skb, DPA_BP_HEAD)) = skb;

		fd.addr_lo = dma_map_single(dev, skb->data, skb_headlen(skb),
				DMA_TO_DEVICE);
		if (unlikely(fd.addr_lo == 0)) {
			cpu_netdev_err(net_dev,
				"%s:%hu:%s(): dma_map_single() failed\n",
				__file__, __LINE__, __func__);
			_errno = -EIO;
			goto _return_dev_kfree_skb;
		}

		fd.length20	= skb_headlen(skb) - DPA_BP_HEAD;
	} else {
		dpa_bp = dpa_size2pool(&priv->dpa_bp_list, skb_headlen(skb));
		BUG_ON(IS_ERR(dpa_bp));

		bmb = (typeof(bmb))&fd;

		_errno = bman_acquire(dpa_bp->pool, bmb, 1, 0);
		if (unlikely(_errno <= 0)) {
			if (netif_msg_tx_err(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): bman_acquire() = %d\n",
					__file__, __LINE__, __func__, _errno);
			net_dev->stats.tx_errors++;
			goto _return_dev_kfree_skb;
		}

		fd.length20	= skb_headlen(skb);
		fd.cmd		= FM_FD_CMD_FCO;

		/* Copy the packet payload */
		skb_copy_from_linear_data(skb,
			dpa_phys2virt(dpa_bp, bmb) + dpa_fd_offset(&fd),
			dpa_fd_length(&fd));
	}

	_errno = qman_enqueue(priv->egress_fqs[skb_get_queue_mapping(skb)],
			&fd, 0);
	if (unlikely(_errno < 0)) {
		if (netif_msg_tx_err(priv))
			cpu_netdev_err(net_dev,
				"%s:%hu:%s(): qman_enqueue() = %d\n",
				__file__, __LINE__, __func__, _errno);
		net_dev->stats.tx_errors++;
		net_dev->stats.tx_fifo_errors++;
		goto _return_buffer;
	}

	net_dev->stats.tx_packets++;
	net_dev->stats.tx_bytes += dpa_fd_length(&fd);

	_errno = NETDEV_TX_OK;

	if (priv->mac_dev)
		goto _return;
	else
		goto _return_dev_kfree_skb;

_return_buffer:
	if (priv->mac_dev)
		dma_unmap_single(dev, fd.addr_lo, skb_headlen(skb),
			DMA_TO_DEVICE);
	else {
		__errno = dpa_fd_release(net_dev, &fd);
		if (unlikely(__errno < 0)) {
			dump_stack();
			panic("Can't release buffer to the BM during a TX\n");
		}
	}
_return_dev_kfree_skb:
	dev_kfree_skb(skb);
_return:
	if (netif_msg_tx_queued(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static int __cold dpa_start(struct net_device *net_dev)
{
	int			 _errno, i=0, j;
	const struct dpa_priv_s	*priv;
	struct mac_device *mac_dev;

	priv = netdev_priv(net_dev);
	mac_dev = priv->mac_dev;

	if (netif_msg_ifup(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	if (mac_dev) {
		_errno = mac_dev->init_phy(net_dev);
		if(_errno) {
			if (netif_msg_ifup(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): init_phy() = %d\n",
					__file__, __LINE__, __func__, _errno);
			goto _return_port_dev_stop;
		}

		for (; i < ARRAY_SIZE(priv->mac_dev->port_dev); i++)
			fm_port_enable(priv->mac_dev->port_dev[i]);

		_errno = priv->mac_dev->start(priv->mac_dev);
		if (unlikely(_errno < 0)) {
			if (netif_msg_ifup(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): mac_dev->start() = %d\n",
					__file__, __LINE__, __func__, _errno);
			goto _return_port_dev_stop;
		}
	}

	netif_tx_start_all_queues(net_dev);

	_errno = 0;
	goto _return;

_return_port_dev_stop:
	for (j = 0; j < i; j++)
		fm_port_disable(priv->mac_dev->port_dev[j]);
_return:
	if (netif_msg_ifup(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static int __cold dpa_stop(struct net_device *net_dev)
{
	int			 _errno, __errno, i;
	const struct dpa_priv_s	*priv;

	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_ifdown(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	netif_tx_stop_all_queues(net_dev);

	_errno = 0;
	if (priv->mac_dev) {
		__errno = priv->mac_dev->stop(priv->mac_dev);
		if (unlikely(__errno < 0)) {
			if (netif_msg_ifdown(priv))
				cpu_netdev_err(net_dev,
					"%s:%hu:%s(): mac_dev->stop() = %d\n",
					__file__, __LINE__, __func__, __errno);
			if (likely(_errno >= 0))
				_errno = __errno;
		}

		for (i = 0; i < ARRAY_SIZE(priv->mac_dev->port_dev); i++)
			fm_port_disable(priv->mac_dev->port_dev[i]);

		phy_disconnect(priv->mac_dev->phy_dev);
		priv->mac_dev->phy_dev = NULL;
	}

	if (netif_msg_ifdown(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static void __cold dpa_timeout(struct net_device *net_dev)
{
	const struct dpa_priv_s	*priv;

	priv = (typeof(priv))netdev_priv(net_dev);

	if (netif_msg_timer(priv))
		cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	if (netif_msg_timer(priv))
		cpu_netdev_crit(net_dev, "Transmit timeout latency: %lu ms\n",
				(jiffies - net_dev->trans_start) * 1000 / HZ);

	net_dev->stats.tx_errors++;
	netif_tx_wake_all_queues(net_dev);

	if (netif_msg_timer(priv))
		cpu_netdev_dbg(net_dev, "%s:%s() ->\n", __file__, __func__);
}

static int __devinit __cold __pure __must_check __attribute__((nonnull))
dpa_bp_cmp(const void *dpa_bp0, const void *dpa_bp1)
{
	return ((struct dpa_bp *)dpa_bp0)->size -
			((struct dpa_bp *)dpa_bp1)->size;
}

static struct dpa_bp * __devinit __cold __must_check __attribute__((nonnull))
dpa_bp_probe(struct of_device *_of_dev, size_t *count)
{
	int			 i, lenp, na, ns;
	struct device		*dev;
	struct device_node	*dev_node;
	const phandle		*phandle_prop;
	const uint32_t		*uint32_prop;
	struct dpa_bp		*dpa_bp;

	dev = &_of_dev->dev;

	/* Get the buffer pools to be used */
	phandle_prop = (typeof(phandle_prop))of_get_property(_of_dev->node,
					"fsl,bman-buffer-pools", &lenp);
	if (phandle_prop == NULL) {
		dpa_bp = NULL;
		goto _return_count;
	}

	*count = lenp / sizeof(phandle);
	dpa_bp = (typeof(dpa_bp))devm_kzalloc(dev, *count * sizeof(*dpa_bp), GFP_KERNEL);
	if (unlikely(dpa_bp == NULL)) {
		cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
			    __file__, __LINE__, __func__);
		dpa_bp = ERR_PTR(-ENOMEM);
		goto _return_count;
	}

	dev_node = of_find_node_by_path("/");
	if (unlikely(dev_node == NULL)) {
		cpu_dev_err(dev,
			"%s:%hu:%s(): of_find_node_by_path(/) failed\n",
			__file__, __LINE__, __func__);
		dpa_bp = ERR_PTR(-EINVAL);
		goto _return_count;
	}

	na = of_n_addr_cells(dev_node);
	ns = of_n_size_cells(dev_node);
	of_node_put(dev_node);

	for (i = 0; i < *count; i++) {
		dev_node = of_find_node_by_phandle(phandle_prop[i]);
		if (unlikely(dev_node == NULL)) {
			cpu_dev_err(dev,
				"%s:%hu:%s(): of_find_node_by_phandle failed\n",
				__file__, __LINE__, __func__);
			return ERR_PTR(-EFAULT);
		}

		if (unlikely(!of_device_is_compatible(dev_node, "fsl,bpool"))) {
			cpu_dev_err(dev, "%s:%hu:%s(): !of_device_is_compatible(%s, fsl,bpool)\n",
				    __file__, __LINE__, __func__, dev_node->full_name);
			dpa_bp = ERR_PTR(-EINVAL);
			goto _return_of_node_put;
		}

		uint32_prop = of_get_property(dev_node, "fsl,bpid", &lenp);
		if (unlikely(uint32_prop == NULL)) {
			cpu_dev_err(dev, "%s:%hu:%s(): of_get_property(%s, fsl,bpid) failed\n",
				    __file__, __LINE__, __func__, dev_node->full_name);
			dpa_bp = ERR_PTR(-EINVAL);
			goto _return_of_node_put;
		}
		BUG_ON(lenp != sizeof(uint32_t));
		dpa_bp[i].bpid = *uint32_prop;

		uint32_prop = of_get_property(dev_node, "fsl,bpool-cfg", &lenp);
		if (uint32_prop != NULL) {
			BUG_ON(lenp != (2 * ns + na) * sizeof(uint32_t));

			dpa_bp[i].count	= of_read_number(uint32_prop, ns);
			dpa_bp[i].size	= of_read_number(uint32_prop + ns, ns);
			dpa_bp[i].paddr	= of_read_number(uint32_prop + 2 * ns,
						na);
		}

		of_node_put(dev_node);
	}

	sort(dpa_bp, *count, sizeof(*dpa_bp), dpa_bp_cmp, NULL);

	goto _return;

_return_of_node_put:
	of_node_put(dev_node);
_return_count:
	*count = 0;
_return:
	return dpa_bp;
}

static struct mac_device * __devinit __cold __must_check
__attribute__((nonnull))
dpa_mac_probe(struct of_device *_of_dev)
{
	struct device		*dpa_dev, *dev;
	struct device_node	*mac_node;
	int			 lenp;
	const phandle		*phandle_prop;
	struct of_device	*of_dev;
	struct mac_device	*mac_dev;

	phandle_prop = (typeof(phandle_prop))of_get_property(_of_dev->node,
				"fsl,fman-mac", &lenp);
	if (phandle_prop == NULL)
		return NULL;

	BUG_ON(lenp != sizeof(phandle));

	dpa_dev = &_of_dev->dev;

	mac_node = of_find_node_by_phandle(*phandle_prop);
	if (unlikely(mac_node == NULL)) {
		cpu_dev_err(dpa_dev,
			"%s:%hu:%s(): of_find_node_by_phandle() failed\n",
			__file__, __LINE__, __func__);
		return ERR_PTR(-EFAULT);
	}

	of_dev = of_find_device_by_node(mac_node);
	if (unlikely(of_dev == NULL)) {
		cpu_dev_err(dpa_dev,
			"%s:%hu:%s(): of_find_device_by_node(%s) failed\n",
			__file__, __LINE__, __func__, mac_node->full_name);
		of_node_put(mac_node);
		return ERR_PTR(-EINVAL);
	}
	of_node_put(mac_node);

	dev = &of_dev->dev;

	mac_dev = (typeof(mac_dev))dev_get_drvdata(dev);
	if (unlikely(mac_dev == NULL)) {
		cpu_dev_err(dpa_dev,
			"%s:%hu:%s(): dev_get_drvdata(%s) failed\n",
			__file__, __LINE__, __func__, dev_name(dev));
		return ERR_PTR(-EINVAL);
	}

	return mac_dev;
}

static const char fsl_qman_frame_queues[][25] __devinitconst = {
	[RX] = "fsl,qman-frame-queues-rx",
	[TX] = "fsl,qman-frame-queues-tx"
};

static int __devinit __cold dpa_alloc_pcd_fqids(struct device	*dev,
						uint32_t	 num,
						uint8_t		 alignment,
						uint32_t	*base_fqid)
{
	int			 _errno, i;
	struct net_device	*net_dev;
	struct dpa_priv_s	*priv;
	struct dpa_fq		*dpa_fq;
	struct qman_fq		*ingress_fq;
	uint32_t			 total_num_fqs = num + alignment;
	uint32_t			 padding = 0;
	uint32_t			prev_fqid;

	net_dev = (typeof(net_dev))dev_get_drvdata(dev);
	priv = (typeof(priv))netdev_priv(net_dev);

	dpa_fq = devm_kzalloc(dev, total_num_fqs * sizeof(*dpa_fq), GFP_KERNEL);
	if (unlikely(dpa_fq == NULL)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
					__file__, __LINE__, __func__);
		_errno = -ENOMEM;
		goto _return;
	}

	for (i = 0, ingress_fq = NULL; i < total_num_fqs; i++, dpa_fq++) {
		dpa_fq->fq_base	= ingress_fqs[RX][1];
		dpa_fq->net_dev	= net_dev;
		prev_fqid = (ingress_fq ? qman_fq_fqid(ingress_fq) : 0);
		ingress_fq = _dpa_fq_alloc(priv->dpa_fq_list + RX, dpa_fq, 0,
				QMAN_FQ_FLAG_NO_ENQUEUE, priv->channel, 7);
		if (IS_ERR(ingress_fq)) {
			_errno = PTR_ERR(ingress_fq);
			goto _return;
		}

		if (prev_fqid && ((prev_fqid - 1) != qman_fq_fqid(ingress_fq))) {
			if (netif_msg_probe(priv))
				cpu_dev_err(dev, "%s:%hu:%s(): Failed to allocate a contiguous range of FQs\n",
					    __file__, __LINE__, __func__);
			_errno = -EINVAL;
			goto _return;
		}

		cpu_dev_dbg(dev, "%s:%s(): ingress_pcd_fqs[%d] = %u\n",
			    __file__, __func__, i, qman_fq_fqid(ingress_fq));
	}

	*base_fqid = qman_fq_fqid(ingress_fq);
	if (alignment)
		padding = alignment - (*base_fqid % alignment);
	*base_fqid += padding;
	BUG_ON((total_num_fqs-padding)<num);
#ifdef CONFIG_FSL_FMAN_TEST
	BUG_ON(priv->num >= (sizeof(priv->ranges)/sizeof(struct pcd_range)));
	priv->ranges[priv->num].base    = *base_fqid;
	priv->ranges[priv->num++].count = num;
#endif /* CONFIG_FSL_FMAN_TEST */

	cpu_dev_dbg(dev, "%s:%s(): pcd_fqs base %u\n", __file__, __func__,
			*base_fqid);

	_errno = 0;

_return:
	cpu_dev_dbg(dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static int __devinit __cold __attribute__((nonnull))
dpa_init_probe(struct of_device *_of_dev)
{
	int				 _errno, i, j, lenp;
	struct device			*dev;
	struct mac_device		*mac_dev;
	struct dpa_bp			*dpa_bp;
	struct fm_port_rx_params	 rx_port_param;
	struct fm_port_non_rx_params	 tx_port_param;
	struct fm_port_pcd_param	 rx_port_pcd_param;
	size_t				 count;
	struct device_node		*dpa_node;
	const uint32_t			*uint32_prop;
	uint32_t	ingress_fqids[ARRAY_SIZE(fsl_qman_frame_queues)][2];

	dev = &_of_dev->dev;

	cpu_dev_dbg(dev, "-> %s:%s()\n", __file__, __func__);

	dpa_node = _of_dev->node;

	if (!of_device_is_available(dpa_node))
		return -ENODEV;

	/* FM */

	mac_dev = dpa_mac_probe(_of_dev);
	if (IS_ERR(mac_dev)) {
		_errno = PTR_ERR(mac_dev);
		goto _return;
	} else if(mac_dev == NULL) {
		cpu_dev_err(dev,
			"%s:%hu:%s(): Missing the %s/fsl,fman-mac property\n",
			__file__, __LINE__, __func__, dpa_node->full_name);
		_errno = -EINVAL;
		goto _return;
	}

	/* BM */

	dpa_bp = dpa_bp_probe(_of_dev, &count);
	if (IS_ERR(dpa_bp)) {
		_errno = PTR_ERR(dpa_bp);
		goto _return;
	} else if (unlikely(dpa_bp == NULL)) {
		cpu_dev_err(dev,
			"%s:%hu: Missing %s/fsl,bman-buffer-pools property\n",
			__file__, __LINE__, dpa_node->full_name);
		_errno = -EINVAL;
		goto _return;
	}

	rx_port_param.num_pools = min(ARRAY_SIZE(rx_port_param.pool_param), count);
	for (i = 0; i < rx_port_param.num_pools; i++) {
		rx_port_param.pool_param[i].id	 = dpa_bp[i].bpid;
		rx_port_param.pool_param[i].size = dpa_bp[i].size;

		cpu_dev_dbg(dev, "%s:%s(): dpa_bp[%d] = {%hu, %u}\n",
				__file__, __func__, i,
				rx_port_param.pool_param[i].id,
				rx_port_param.pool_param[i].size);
	}
	devm_kfree(dev, dpa_bp);

	/* QM */

	for (i = 0; i < ARRAY_SIZE(ingress_fqids); i++) {
		uint32_prop = (typeof(uint32_prop))of_get_property(dpa_node,
				fsl_qman_frame_queues[i], &lenp);
		if (unlikely(uint32_prop == NULL)) {
			cpu_dev_info(dev,
				"%s:%hu:%s(): of_get_property(%s, %s) failed\n",
				__file__, __LINE__, __func__,
				dpa_node->full_name, fsl_qman_frame_queues[i]);
			_errno = -EINVAL;
			goto _return;
		}
		BUG_ON(lenp != ARRAY_SIZE(ingress_fqids[i]) * 2 * sizeof(uint32_t));

		for (j = 0; j < ARRAY_SIZE(ingress_fqids[i]); j++) {
			ingress_fqids[i][j] = uint32_prop[j * 2];
			BUG_ON(uint32_prop[j * 2 + 1] != 1);
		}
	}

	/* Set error FQs */
	rx_port_param.errq	= ingress_fqids[RX][0];
	tx_port_param.errq	= ingress_fqids[TX][0];

	/* Set default FQs */
	rx_port_param.defq	= ingress_fqids[RX][1];
	tx_port_param.defq	= ingress_fqids[TX][1];

	rx_port_param.priv_data_size	= tx_port_param.priv_data_size	= 32;
	rx_port_param.parse_results	= tx_port_param.parse_results	= true;

	fm_set_rx_port_params(mac_dev->port_dev[RX], &rx_port_param);

	/* Set PCD FQs */
	rx_port_pcd_param.cb	= dpa_alloc_pcd_fqids;
	rx_port_pcd_param.dev	= dev;
	fm_port_pcd_bind(mac_dev->port_dev[RX], &rx_port_pcd_param);

	fm_set_tx_port_params(mac_dev->port_dev[TX], &tx_port_param);

	for (i = 0; i < ARRAY_SIZE(mac_dev->port_dev); i++)
		fm_port_enable(mac_dev->port_dev[i]);

	_errno = 0;

_return:
	cpu_dev_dbg(dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static const uint32_t *dpa_get_fqids(struct device_node *dpa_node,
					const char *match, int *num)
{
	const uint32_t *fqids;

	fqids = of_get_property(dpa_node, match, num);
	if (unlikely(!fqids)) {
		printk(KERN_ERR "%s: of_get_property(%s) failed\n",
				dpa_node->full_name, match);

		*num = 0;
		return NULL;
	}

	*num /= (2 * sizeof(*fqids));

	return fqids;
}

static int dpa_count_fqs(const uint32_t *fqids, int num)
{
	int i;
	int count = 0;

	for (i = 0; i < num; i++)
		count += fqids[2 * i + 1];

	return count;
}

static int __devinit __cold __attribute__((nonnull))
dpa_probe(struct of_device *_of_dev)
{
	int				 _errno, i, j, lenp;
	struct device			*dev;
	struct device_node		*dpa_node, *dev_node;
	struct net_device		*net_dev;
	struct dpa_priv_s		*priv;
	struct dpa_bp			*dpa_bp;
	struct dpa_fq			*dpa_fq;
	struct fd_list_head		*fd_list;
	struct fm_port_rx_params	 rx_port_param;
	struct fm_port_non_rx_params	 tx_port_param;
	struct fm_port_pcd_param	 rx_port_pcd_param;
	size_t				 count;
	const phandle			*phandle_prop;
	const uint32_t			*uint32_prop;
	const uint8_t			*mac_addr;
	struct qman_fq			*ingress_fq;
	uint32_t		ingress_fqids[ARRAY_SIZE(ingress_fqs)][2];
	const uint32_t		default_tx_fqids[6] = {0, 1, 0, 1, 0, 8};
	const uint32_t		default_rx_fqids[6] = {0, 1, 0, 1, 0, 8};
	const uint32_t		*tx_fqids;
	const uint32_t		*rx_fqids;
	int			num_tx_fqids, num_tx_fqs;
	int			num_rx_fqids, num_rx_fqs;

	dev = &_of_dev->dev;

	cpu_dev_dbg(dev, "-> %s:%s()\n", __file__, __func__);

	dpa_node = _of_dev->node;

	if (!of_device_is_available(dpa_node))
		return -ENODEV;

	/*
	 * Allocate this early, so we can store relevant information in
	 * the private area
	 */
	net_dev = alloc_etherdev_mq(sizeof(*priv),
			ARRAY_SIZE(priv->egress_fqs));
	if (unlikely(net_dev == NULL)) {
		cpu_dev_err(dev, "%s:%hu:%s(): alloc_etherdev_mq() failed\n",
				__file__, __LINE__, __func__);
		_errno = -ENOMEM;
		goto _return;
	}

	/* Do this here, so we can be verbose early */
	SET_NETDEV_DEV(net_dev, dev);
	dev_set_drvdata(dev, net_dev);

	priv = (typeof(priv))netdev_priv(net_dev);
	priv->net_dev = net_dev;

	priv->msg_enable = netif_msg_init(debug, -1);

	/* BM */

	dpa_bp = dpa_bp_probe(_of_dev, &count);
	if (IS_ERR(dpa_bp)) {
		_errno = PTR_ERR(dpa_bp);
		goto _return_free_netdev;
	} else if (dpa_bp == NULL) {
		if (netif_msg_probe(priv))
			cpu_dev_info(dev,
				"%s:%hu:%s(): Using private BM buffer pools\n",
				__file__, __LINE__, __func__);

		count = ARRAY_SIZE(dpa_bp_size);
		dpa_bp = devm_kzalloc(dev, count * sizeof(*dpa_bp), GFP_KERNEL);
		if (unlikely(dpa_bp == NULL)) {
			cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
					__file__, __LINE__, __func__);
			_errno = -ENOMEM;
			goto _return_free_netdev;
		}

		for (i = 0; i < count; i++) {
			dpa_bp[i].count	= 128;
			dpa_bp[i].size	= dpa_bp_size[i];
		}
	} else if (count == ARRAY_SIZE(dpa_bp_size)) {
		for (i = 0, j = 0; i < count; i++) {
			if (dpa_bp[i].count == 0 && dpa_bp[i].size == 0 &&
					dpa_bp[i].paddr == 0) {
				dpa_bp[i].count	= 128;
				dpa_bp[i].size	= dpa_bp_size[i];
				j++;
			}
		}
		BUG_ON(j > 0 && j < count);
	}

	INIT_LIST_HEAD(&priv->dpa_bp_list);

	for (i = 0; i < count; i++) {
		_errno = _dpa_bp_alloc(net_dev, &priv->dpa_bp_list, dpa_bp + i);
		if (unlikely(_errno < 0))
			goto _return_dpa_bp_free;
	}

	/* QM */

	phandle_prop = (typeof(phandle_prop))of_get_property(dpa_node,
				"fsl,qman-channel", &lenp);
	if (unlikely(phandle_prop == NULL)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev, "%s:%hu:%s(): of_get_property(%s, fsl,qman-channel) failed\n",
				    __file__, __LINE__, __func__, dpa_node->full_name);
		_errno = -EINVAL;
		goto _return_dpa_bp_free;
	}
	BUG_ON(lenp != sizeof(phandle));

	dev_node = of_find_node_by_phandle(*phandle_prop);
	if (unlikely(dev_node == NULL)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev,
				"%s:%hu:%s: of_find_node_by_phandle() failed\n",
				__file__, __LINE__, __func__);
		_errno = -EFAULT;
		goto _return_dpa_bp_free;
	}

	uint32_prop = (typeof(uint32_prop))of_get_property(dev_node,
			"fsl,qman-channel-id", &lenp);
	if (unlikely(uint32_prop == NULL)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev, "%s:%hu:%s(): of_get_property(%s, fsl,qman-channel-id) failed\n",
				    __file__, __LINE__, __func__, dpa_node->full_name);
		of_node_put(dev_node);
		_errno = -EINVAL;
		goto _return_dpa_bp_free;
	}
	of_node_put(dev_node);
	BUG_ON(lenp != sizeof(uint32_t));
	priv->channel = *uint32_prop;

	for (i = 0; i < ARRAY_SIZE(priv->dpa_fq_list); i++)
		INIT_LIST_HEAD(priv->dpa_fq_list + i);

	INIT_WORK(&priv->fd_work, dpa_rx);

	priv->fd_list = __alloc_percpu(sizeof(*priv->fd_list),
				__alignof__(*priv->fd_list));
	if (unlikely(priv->fd_list == NULL)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev,
				"%s:%hu:%s(): __alloc_percpu() failed\n",
				__file__, __LINE__, __func__);
		_errno = -ENOMEM;
		goto _return_dpa_bp_free;
	}

	for_each_online_cpu(i) {
		fd_list = per_cpu_ptr(priv->fd_list, i);

		INIT_LIST_HEAD(&fd_list->list);
		fd_list->count	= 0;
		fd_list->max	= 0;
	}

	/* FM */

	priv->mac_dev = dpa_mac_probe(_of_dev);
	if (IS_ERR(priv->mac_dev)) {
		_errno = PTR_ERR(priv->mac_dev);
		goto _return_free_percpu;
	}

	tx_fqids = dpa_get_fqids(dpa_node, "fsl,qman-frame-queues-tx",
					&num_tx_fqids);
	num_tx_fqs = dpa_count_fqs(tx_fqids, num_tx_fqids);
	rx_fqids = dpa_get_fqids(dpa_node, "fsl,qman-frame-queues-rx",
					&num_rx_fqids);
	num_rx_fqs = dpa_count_fqs(rx_fqids, num_rx_fqids);

	if(priv->mac_dev == NULL) {
		if (netif_msg_probe(priv))
			cpu_dev_info(dev,
				"%s:%hu: Missing the %s/fsl,fman-mac property. "
					  "This is a MAC-less interface\n",
					  __file__, __LINE__,
					  dpa_node->full_name);

		/* Get the MAC address */
		mac_addr = of_get_mac_address(dpa_node);
		if (unlikely(mac_addr == NULL)) {
			cpu_dev_err(dev,
				"%s:%hu:%s(): of_get_mac_address(%s) failed\n",
				__file__, __LINE__, __func__,
				dpa_node->full_name);
			_errno = -EINVAL;
			goto _return_free_percpu;
		}

		memcpy(net_dev->perm_addr, mac_addr, net_dev->addr_len);
		memcpy(net_dev->dev_addr, mac_addr, net_dev->addr_len);

		/* QM */

		/* For now, stick to the idea that we have to have
		 * static declarations on macless devices */
		if (!tx_fqids || !rx_fqids) {
			cpu_dev_err(dev,
				"macless devices require fq declarations\n");
			_errno = -EINVAL;
			goto _return_free_percpu;
		}

		dpa_fq = devm_kzalloc(dev, sizeof(*dpa_fq) * num_rx_fqs,
					GFP_KERNEL);
		if (unlikely(dpa_fq == NULL)) {
			cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
					__file__, __LINE__, __func__);
			_errno = -ENOMEM;
			goto _return_free_percpu;
		}

		for (i = 0; i < num_rx_fqids; i++) {
			for (j = 0; j < rx_fqids[2 * i + 1]; j++, dpa_fq++) {
				uint32_t fqid =
					rx_fqids[2 * i] ?
					rx_fqids[2 * i] + j : 0;
				/* The work queue will be set to the value
				 * of the fqid mod 8.  This way, system
				 * architects can choose the priority
				 * of the frame queue by statically
				 * assigning the fqid
				 */
				int wq = fqid ? fqid % 8 : 7;

				dpa_fq->fq_base = ingress_fqs[RX][1];
				dpa_fq->net_dev	= net_dev;
				ingress_fq = _dpa_fq_alloc(
						priv->dpa_fq_list + RX,
						dpa_fq, fqid,
						QMAN_FQ_FLAG_NO_ENQUEUE,
						priv->channel, wq);
				if (IS_ERR(ingress_fq)) {
					_errno = PTR_ERR(ingress_fq);
					goto _return_dpa_fq_free;
				}


				cpu_dev_dbg(dev,
					"%s:%s(): ingress_fqs[%d][%d] = %u\n",
					__file__, __func__, i, j,
					qman_fq_fqid(ingress_fq));
			}
		}

		/* Right now, we maintain the requirement that we have 8 */
		BUG_ON(num_tx_fqs != 8);

		/* FIXME: Horribly leaky */
		dpa_fq = devm_kzalloc(dev, sizeof(*dpa_fq) * num_tx_fqs,
					GFP_KERNEL);
		if (unlikely(dpa_fq == NULL)) {
			cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
					__file__, __LINE__, __func__);
			_errno = -ENOMEM;
			goto _return_free_percpu;
		}

		for (i = 0; i < tx_fqids[1]; i++, dpa_fq++) {
			uint32_t fqid = tx_fqids[0] ? tx_fqids[0] + i : 0;

			dpa_fq->fq_base = _egress_fqs;
			dpa_fq->net_dev	= net_dev;
			priv->egress_fqs[i] = _dpa_fq_alloc(
					priv->dpa_fq_list + TX, dpa_fq, fqid,
					QMAN_FQ_FLAG_NO_MODIFY, 0, 0);
			if (IS_ERR(priv->egress_fqs[i])) {
				_errno = PTR_ERR(priv->egress_fqs[i]);
				goto _return_dpa_fq_free;
			}

			cpu_dev_dbg(dev, "%s:%s(): egress_fqs[%d] = %u\n",
					__file__, __func__, i,
					qman_fq_fqid(priv->egress_fqs[i]));
		}
	} else {
		uint32_t ingress_fq_cfg[2][2];

		net_dev->mem_start	= priv->mac_dev->res->start;
		net_dev->mem_end	= priv->mac_dev->res->end;

		memcpy(net_dev->perm_addr, priv->mac_dev->addr,
			net_dev->addr_len);
		memcpy(net_dev->dev_addr, priv->mac_dev->addr,
			net_dev->addr_len);

		/* BM */
		rx_port_param.num_pools =
			min(ARRAY_SIZE(rx_port_param.pool_param), count);
		i = 0;
		list_for_each_entry(dpa_bp, &priv->dpa_bp_list, list) {
			if (i >= rx_port_param.num_pools)
				break;

			rx_port_param.pool_param[i].id = dpa_pool2bpid(dpa_bp);
			rx_port_param.pool_param[i].size = dpa_bp->size;

			cpu_dev_dbg(dev, "%s:%s(): dpa_bp[%d] = {%hu, %u}\n",
					__file__, __func__,
					i, rx_port_param.pool_param[i].id,
					rx_port_param.pool_param[i].size);

			i++;
		}

		/* QM */

		if (!tx_fqids) {
			tx_fqids = &default_tx_fqids[0];
			num_tx_fqids = ARRAY_SIZE(default_tx_fqids) / 2;
			num_tx_fqs = dpa_count_fqs(tx_fqids, num_tx_fqids);
		}

		if (!rx_fqids) {
			rx_fqids = &default_rx_fqids[0];
			num_rx_fqids = ARRAY_SIZE(default_rx_fqids) / 2;
			num_rx_fqs = dpa_count_fqs(rx_fqids, num_rx_fqids);
		}

		/* FIXME: Horribly leaky */
		dpa_fq = devm_kzalloc(dev,
				sizeof(*dpa_fq) * (num_tx_fqs + num_rx_fqs),
				GFP_KERNEL);
		if (unlikely(dpa_fq == NULL)) {
			cpu_dev_err(dev, "%s:%hu:%s(): devm_kzalloc() failed\n",
					__file__, __LINE__, __func__);
			_errno = -ENOMEM;
			goto _return_free_percpu;
		}

		/* device tree has default,error, but local array is reversed */
		ingress_fq_cfg[0][1] = rx_fqids[0];
		ingress_fq_cfg[0][0] = rx_fqids[2];
		ingress_fq_cfg[1][1] = tx_fqids[0];
		ingress_fq_cfg[1][0] = tx_fqids[2];

		for (i = 0; i < ARRAY_SIZE(ingress_fqs); i++) {
			/* Error, default */
			for (j = 0; j < ARRAY_SIZE(ingress_fqs[i]); j++,
					dpa_fq++) {
				dpa_fq->fq_base	= ingress_fqs[i][j];
				dpa_fq->net_dev	= net_dev;
				ingress_fq = _dpa_fq_alloc(
						priv->dpa_fq_list + RX,
						dpa_fq, ingress_fq_cfg[i][j],
						QMAN_FQ_FLAG_NO_ENQUEUE,
						priv->channel, 7);
				if (IS_ERR(ingress_fq)) {
					_errno = PTR_ERR(ingress_fq);
					goto _return_dpa_fq_free;
				}

				ingress_fqids[i][j] = qman_fq_fqid(ingress_fq);

				cpu_dev_dbg(dev,
					"%s:%s(): ingress_fqs[%s][%d] = %u\n",
					__file__, __func__, rtx[i], j,
					ingress_fqids[i][j]);
			}
		}

		/* Loop through the remaining fq assignments */
		for (i = 2; i < num_rx_fqids; i++) {
			for (j = 0; j < rx_fqids[2 * i + 1]; j++, dpa_fq++) {
				uint32_t fqid =
					rx_fqids[2 * i] ?
					rx_fqids[2 * i] + j : 0;
				int wq = fqid ? fqid % 8 : 7;

				dpa_fq->fq_base = ingress_fqs[RX][1];
				dpa_fq->net_dev	= net_dev;
				ingress_fq = _dpa_fq_alloc(
						priv->dpa_fq_list + RX,
						dpa_fq, fqid,
						QMAN_FQ_FLAG_NO_ENQUEUE,
						priv->channel, wq);
				if (IS_ERR(ingress_fq)) {
					_errno = PTR_ERR(ingress_fq);
					goto _return_dpa_fq_free;
				}

				cpu_dev_dbg(dev,
					"%s:%s(): ingress_fqs[%d][%d] = %u\n",
					__file__, __func__, i, j,
					qman_fq_fqid(ingress_fq));
			}
		}

		/* We only support 8 for now */
		BUG_ON(tx_fqids[5] != 8);

		for (i = 0; i < ARRAY_SIZE(priv->egress_fqs); i++, dpa_fq++) {
			uint32_t fqid = tx_fqids[4] ? tx_fqids[4] + i : 0;

			dpa_fq->fq_base	= _egress_fqs;
			dpa_fq->net_dev	= net_dev;
			priv->egress_fqs[i] = _dpa_fq_alloc(
				priv->dpa_fq_list + TX, dpa_fq, fqid,
				QMAN_FQ_FLAG_TO_DCPORTAL,
				fm_get_tx_port_channel(priv->mac_dev->port_dev[TX]), i);
			if (IS_ERR(priv->egress_fqs[i])) {
				_errno = PTR_ERR(priv->egress_fqs[i]);
				goto _return_dpa_fq_free;
			}

			cpu_dev_dbg(dev, "%s:%s(): egress_fqs[%d] = %u\n",
					__file__, __func__, i,
					qman_fq_fqid(priv->egress_fqs[i]));
		}

		net_dev->mem_start	= priv->mac_dev->res->start;
		net_dev->mem_end	= priv->mac_dev->res->end;

		memcpy(net_dev->perm_addr, priv->mac_dev->addr,
			net_dev->addr_len);
		memcpy(net_dev->dev_addr, priv->mac_dev->addr,
			net_dev->addr_len);

		/* Set error FQs */
		rx_port_param.errq	= ingress_fqids[RX][0];
		tx_port_param.errq	= ingress_fqids[TX][0];

		/* Set default FQs */
		rx_port_param.defq	= ingress_fqids[RX][1];
		tx_port_param.defq	= ingress_fqids[TX][1];

		rx_port_param.priv_data_size =
			 tx_port_param.priv_data_size = 32;
		rx_port_param.parse_results =
			tx_port_param.parse_results = true;

		fm_set_rx_port_params(priv->mac_dev->port_dev[RX],
			&rx_port_param);

		/* Set PCD FQs */
		rx_port_pcd_param.cb	= dpa_alloc_pcd_fqids;
		rx_port_pcd_param.dev	= dev;
		fm_port_pcd_bind(priv->mac_dev->port_dev[RX],
			&rx_port_pcd_param);

		fm_set_tx_port_params(priv->mac_dev->port_dev[TX],
			&tx_port_param);
	}

	net_dev->features		|= DPA_NETIF_FEATURES;
	net_dev->get_stats		 = dpa_get_stats;
	SET_ETHTOOL_OPS(net_dev, &dpa_ethtool_ops);
	net_dev->needed_headroom	 = DPA_BP_HEAD;
	net_dev->hard_start_xmit	 = dpa_tx;
	net_dev->watchdog_timeo		 = tx_timeout * HZ / 1000;
	net_dev->open			 = dpa_start;
	net_dev->stop			 = dpa_stop;
	net_dev->tx_timeout		 = dpa_timeout;
	net_dev->change_rx_flags	 = dpa_change_rx_flags;

	_errno = register_netdev(net_dev);
	if (unlikely(_errno < 0)) {
		if (netif_msg_probe(priv))
			cpu_dev_err(dev,
				"%s:%hu:%s(): register_netdev() = %d\n",
				__file__, __LINE__, __func__, _errno);
		goto _return_dpa_fq_free;
	}

	goto _return;

_return_dpa_fq_free:
	for (i = 0; i < ARRAY_SIZE(priv->dpa_fq_list); i++)
		dpa_fq_free(dev, priv->dpa_fq_list + i);
_return_free_percpu:
	free_percpu(priv->fd_list);
_return_dpa_bp_free:
	dpa_bp_free(dev, &priv->dpa_bp_list);
_return_free_netdev:
	dev_set_drvdata(dev, NULL);
	free_netdev(net_dev);
_return:
	cpu_dev_dbg(dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static const struct of_device_id dpa_match[] __devinitconst = {
	{
		.compatible	= "fsl,dpa-ethernet-init"
	},
	{
		.compatible	= "fsl,dpa-ethernet"
	},
	{}
};
MODULE_DEVICE_TABLE(of, dpa_match);

static int __devinit __cold
_dpa_probe(struct of_device *_of_dev, const struct of_device_id *match)
{
	return match ==
		dpa_match ? dpa_init_probe(_of_dev) : dpa_probe(_of_dev);
}

static int __devexit __cold dpa_remove(struct of_device *of_dev)
{
	int			 _errno, __errno, i;
	struct device		*dev;
	struct net_device	*net_dev;
	struct dpa_priv_s	*priv;

	dev = &of_dev->dev;
	net_dev = (typeof(net_dev))dev_get_drvdata(dev);

	cpu_netdev_dbg(net_dev, "-> %s:%s()\n", __file__, __func__);

	dev_set_drvdata(dev, NULL);
	unregister_netdev(net_dev);

	priv = (typeof(priv))netdev_priv(net_dev);

	for (i = 0, _errno = 0; i < ARRAY_SIZE(priv->dpa_fq_list); i++) {
		__errno = dpa_fq_free(dev, priv->dpa_fq_list + i);
		if (unlikely(__errno < 0) && _errno >= 0)
			_errno = __errno;
	}

	free_percpu(priv->fd_list);

	dpa_bp_free(dev, &priv->dpa_bp_list);

	free_netdev(net_dev);

	cpu_dev_dbg(dev, "%s:%s() ->\n", __file__, __func__);

	return _errno;
}

static struct of_platform_driver dpa_driver = {
	.name		= KBUILD_MODNAME,
	.match_table	= dpa_match,
	.owner		= THIS_MODULE,
	.probe		= _dpa_probe,
	.remove		= __devexit_p(dpa_remove)
};

static int __init __cold dpa_load(void)
{
	int	 _errno;

	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	cpu_pr_info(KBUILD_MODNAME ": " DPA_DESCRIPTION " (" VERSION ")\n");

	_errno = of_register_platform_driver(&dpa_driver);
	if (unlikely(_errno < 0)) {
		cpu_pr_err(KBUILD_MODNAME
			": %s:%hu:%s(): of_register_platform_driver() = %d\n",
			__file__, __LINE__, __func__, _errno);
		goto _return;
	}

	goto _return;

_return:
	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);

	return _errno;
}
module_init(dpa_load);

static void __exit __cold dpa_unload(void)
{
	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	of_unregister_platform_driver(&dpa_driver);

	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);
}
module_exit(dpa_unload);
