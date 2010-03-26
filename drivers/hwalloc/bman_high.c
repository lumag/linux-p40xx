/* Copyright (c) 2008, 2009 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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

/* TODO:
 *
 * - make RECOVER also handle incomplete mgmt-commands
 */

#include "bman_private.h"

/* Compilation constants */
#define RCR_THRESH	2	/* reread h/w CI when running out of space */
#define RCR_ITHRESH	4	/* if RCR congests, interrupt threshold */

/**************/
/* Portal API */
/**************/

struct bman_portal {
	struct bm_portal *p;
	/* 2-element array. pools[0] is mask, pools[1] is snapshot. */
	struct bman_depletion *pools;
	u32 flags;	/* BMAN_PORTAL_FLAG_*** - static, caller-provided */
	int thresh_set;
	u32 slowpoll;	/* only used when interrupts are off */
	wait_queue_head_t queue;
	/* The wrap-around rcr_[prod|cons] counters are used to support
	 * BMAN_RELEASE_FLAG_WAIT_SYNC. */
	u32 rcr_prod, rcr_cons;
	/* 64-entry hash-table of pool objects that are tracking depletion
	 * entry/exit (ie. BMAN_POOL_FLAG_DEPLETION). This isn't fast-path, so
	 * we're not fussy about cache-misses and so forth - whereas the above
	 * members should all fit in one cacheline.
	 * BTW, with 64 entries in the hash table and 64 buffer pools to track,
	 * you'll never guess the hash-function ... */
	struct bman_pool *cb[64];
};

/* GOTCHA: this object type refers to a pool, it isn't *the* pool. There may be
 * more than one such object per Bman buffer pool, eg. if different users of the
 * pool are operating via different portals. */
struct bman_pool {
	struct bman_pool_params params;
	/* Used for hash-table admin when using depletion notifications. */
	struct bman_portal *portal;
	struct bman_pool *next;
	/* stockpile state - NULL unless BMAN_POOL_FLAG_STOCKPILE is set */
	struct bm_buffer *sp;
	unsigned int sp_fill;
};

/* (De)Registration of depletion notification callbacks */
static void depletion_link(struct bman_portal *portal, struct bman_pool *pool)
{
	pool->portal = portal;
	local_irq_disable();
	pool->next = portal->cb[pool->params.bpid];
	portal->cb[pool->params.bpid] = pool;
	if (!pool->next)
		/* First object for that bpid on this portal, enable the BSCN
		 * mask bit. */
		bm_isr_bscn_mask(portal->p, pool->params.bpid, 1);
	local_irq_enable();
}
static void depletion_unlink(struct bman_pool *pool)
{
	struct bman_pool *it, *last = NULL;
	struct bman_pool **base = &pool->portal->cb[pool->params.bpid];
	local_irq_disable();
	it = *base;	/* <-- gotcha, don't do this prior to the irq_disable */
	while (it != pool) {
		last = it;
		it = it->next;
	}
	if (!last)
		*base = pool->next;
	else
		last->next = pool->next;
	if (!last && !pool->next)
		/* Last object for that bpid on this portal, disable the BSCN
		 * mask bit. */
		bm_isr_bscn_mask(pool->portal->p, pool->params.bpid, 0);
	local_irq_enable();
}

static u32 __poll_portal_slow(struct bman_portal *p);
static void __poll_portal_fast(struct bman_portal *p);

/* Portal interrupt handler */
static irqreturn_t portal_isr(int irq, void *ptr)
{
	struct bman_portal *p = ptr;
#ifdef CONFIG_FSL_BMAN_CHECKING
	if (unlikely(!(p->flags & BMAN_PORTAL_FLAG_IRQ))) {
		pr_crit("Portal interrupt is supposed to be disabled!\n");
		bm_isr_inhibit(p->p);
		return IRQ_HANDLED;
	}
#endif
	/* Only do fast-path handling if it's required */
	if (p->flags & BMAN_PORTAL_FLAG_IRQ_FAST)
		__poll_portal_fast(p);
	__poll_portal_slow(p);
	return IRQ_HANDLED;
}

struct bman_portal *bman_create_portal(struct bm_portal *__p,
			u32 flags, const struct bman_depletion *pools)
{
	struct bman_portal *portal;
	const struct bm_portal_config *config = bm_portal_config(__p);
	int ret;

	portal = kmalloc(sizeof(*portal), GFP_KERNEL);
	if (!portal)
		return NULL;
	if (bm_rcr_init(__p, bm_rcr_pvb, bm_rcr_cci)) {
		pr_err("Bman RCR initialisation failed\n");
		goto fail_rcr;
	}
	if (bm_mc_init(__p)) {
		pr_err("Bman MC initialisation failed\n");
		goto fail_mc;
	}
	if (bm_isr_init(__p)) {
		pr_err("Bman ISR initialisation failed\n");
		goto fail_isr;
	}
	portal->p = __p;
	if (!pools)
		portal->pools = NULL;
	else {
		u8 bpid = 0;
		portal->pools = kmalloc(2 * sizeof(*pools), GFP_KERNEL);
		if (!portal->pools)
			goto fail_pools;
		portal->pools[0] = *pools;
		bman_depletion_init(portal->pools + 1);
		while (bpid < 64) {
			/* Default to all BPIDs disabled, we enable as required
			 * at run-time. */
			bm_isr_bscn_mask(__p, bpid, 0);
			bpid++;
		}
	}
	portal->flags = flags;
	portal->slowpoll = 0;
	init_waitqueue_head(&portal->queue);
	portal->rcr_prod = portal->rcr_cons = 0;
	memset(&portal->cb, 0, sizeof(portal->cb));
	/* Write-to-clear any stale interrupt status bits */
	bm_isr_disable_write(portal->p, 0xffffffff);
	bm_isr_enable_write(portal->p, BM_PIRQ_RCRI | BM_PIRQ_BSCN);
	bm_isr_status_clear(portal->p, 0xffffffff);
	if (flags & BMAN_PORTAL_FLAG_IRQ) {
		if (request_irq(config->irq, portal_isr, 0, "Bman portal 0", portal)) {
			pr_err("request_irq() failed\n");
			goto fail_irq;
		}
		if ((config->cpu != -1) &&
				irq_can_set_affinity(config->irq) &&
				irq_set_affinity(config->irq,
				     cpumask_of(config->cpu))) {
			pr_err("irq_set_affinity() failed\n");
			goto fail_affinity;
		}
		/* Enable the bits that make sense */
		bm_isr_uninhibit(portal->p);
	} else
		/* without IRQ, we can't block */
		flags &= ~BMAN_PORTAL_FLAG_WAIT;
	/* Need RCR to be empty before continuing */
	bm_isr_disable_write(portal->p, ~BM_PIRQ_RCRI);
	if (!(flags & BMAN_PORTAL_FLAG_RECOVER) ||
			!(flags & BMAN_PORTAL_FLAG_WAIT))
		ret = bm_rcr_get_fill(portal->p);
	else if (flags & BMAN_PORTAL_FLAG_WAIT_INT)
		ret = wait_event_interruptible(portal->queue,
			!bm_rcr_get_fill(portal->p));
	else {
		wait_event(portal->queue, !bm_rcr_get_fill(portal->p));
		ret = 0;
	}
	if (ret) {
		pr_err("Bman RCR unclean, need recovery\n");
		goto fail_rcr_empty;
	}
	bm_isr_disable_write(portal->p, 0);
	return portal;
fail_rcr_empty:
fail_affinity:
	if (flags & BMAN_PORTAL_FLAG_IRQ)
		free_irq(config->irq, portal);
fail_irq:
	if (portal->pools)
		kfree(portal->pools);
fail_pools:
	bm_isr_finish(__p);
fail_isr:
	bm_mc_finish(__p);
fail_mc:
	bm_rcr_finish(__p);
fail_rcr:
	kfree(portal);
	return NULL;
}

void bman_destroy_portal(struct bman_portal *bm)
{
	bm_rcr_cci_update(bm->p);
	if (bm->flags & BMAN_PORTAL_FLAG_IRQ)
		free_irq(bm_portal_config(bm->p)->irq, bm);
	if (bm->pools)
		kfree(bm->pools);
	bm_isr_finish(bm->p);
	bm_mc_finish(bm->p);
	bm_rcr_finish(bm->p);
	kfree(bm);
}

/* When release logic waits on available RCR space, we need a global waitqueue
 * in the case of "affine" use (as the waits wake on different cpus which means
 * different portals - so we can't wait on any per-portal waitqueue). */
static DECLARE_WAIT_QUEUE_HEAD(affine_queue);

static u32 __poll_portal_slow(struct bman_portal *p)
{
	struct bman_depletion tmp;
	u32 ret, is = bm_isr_status_read(p->p);
	ret = is;

	/* There is a gotcha to be aware of. If we do the query before clearing
	 * the status register, we may miss state changes that occur between the
	 * two. If we write to clear the status register before the query, the
	 * cache-enabled query command may overtake the status register write
	 * unless we use a heavyweight sync (which we don't want). Instead, we
	 * write-to-clear the status register then *read it back* before doing
	 * the query, hence the odd while loop with the 'is' accumulation. */
	if (is & BM_PIRQ_BSCN) {
		struct bm_mc_result *mcr;
		unsigned int i, j;
		u32 __is;
		bm_isr_status_clear(p->p, BM_PIRQ_BSCN);
		while ((__is = bm_isr_status_read(p->p)) & BM_PIRQ_BSCN) {
			is |= __is;
			bm_isr_status_clear(p->p, BM_PIRQ_BSCN);
		}
		is &= ~BM_PIRQ_BSCN;
		local_irq_disable();
		bm_mc_start(p->p);
		bm_mc_commit(p->p, BM_MCC_VERB_CMD_QUERY);
		while (!(mcr = bm_mc_result(p->p)))
			cpu_relax();
		tmp = mcr->query.ds.state;
		local_irq_enable();
		for (i = 0; i < 2; i++) {
			int idx = i * 32;
			/* tmp is a mask of currently-depleted pools.
			 * pools[0] is mask of those we care about.
			 * pools[1] is our previous view (we only want to
			 * be told about changes). */
			tmp.__state[i] &= p->pools[0].__state[i];
			if (tmp.__state[i] == p->pools[1].__state[i])
				/* fast-path, nothing to see, move along */
				continue;
			for (j = 0; j <= 31; j++, idx++) {
				struct bman_pool *pool = p->cb[idx];
				int b4 = bman_depletion_get(&p->pools[1], idx);
				int af = bman_depletion_get(&tmp, idx);
				if (b4 == af)
					continue;
				while (pool) {
					pool->params.cb(p, pool,
						pool->params.cb_ctx, af);
					pool = pool->next;
				}
			}
		}
		p->pools[1] = tmp;
	}

	if (is & BM_PIRQ_RCRI) {
		local_irq_disable();
		p->rcr_cons += bm_rcr_cci_update(p->p);
		bm_rcr_set_ithresh(p->p, 0);
		wake_up(&p->queue);
		local_irq_enable();
		bm_isr_status_clear(p->p, BM_PIRQ_RCRI);
		is &= ~BM_PIRQ_RCRI;
	}

	/* There should be no status register bits left undefined */
	BM_ASSERT(!is);
	return ret;
}

static void __poll_portal_fast(struct bman_portal *p)
{
	/* nothing yet, this is where we'll put optimised RCR consumption
	 * tracking */
}

/* In the case that slow- and fast-path handling are both done by bman_poll()
 * (ie. because there is no interrupt handling), we ought to balance how often
 * we do the fast-path poll versus the slow-path poll. We'll use two decrementer
 * sources, so we call the fast poll 'n' times before calling the slow poll
 * once. The idle decrementer constant is used when the last slow-poll detected
 * no work to do, and the busy decrementer constant when the last slow-poll had
 * work to do. */
#define SLOW_POLL_IDLE   1000
#define SLOW_POLL_BUSY   10
void bman_poll(void)
{
	struct bman_portal *p = get_affine_portal();
	if (!(p->flags & BMAN_PORTAL_FLAG_IRQ)) {
		/* we handle slow- and fast-path */
		__poll_portal_fast(p);
		if (!(p->slowpoll--)) {
			u32 active = __poll_portal_slow(p);
			if (active)
				p->slowpoll = SLOW_POLL_BUSY;
			else
				p->slowpoll = SLOW_POLL_IDLE;
		}
	} else if (!(p->flags & BMAN_PORTAL_FLAG_IRQ_FAST))
		/* we handle fast-path only */
		__poll_portal_fast(p);
	put_affine_portal();
}
EXPORT_SYMBOL(bman_poll);

static const u32 zero_thresholds[4] = {0, 0, 0, 0};

struct bman_pool *bman_new_pool(const struct bman_pool_params *params)
{
	struct bman_pool *pool = NULL;
	u32 bpid;

	if (params->flags & BMAN_POOL_FLAG_DYNAMIC_BPID) {
#ifdef CONFIG_FSL_BMAN_CONFIG
		int ret = bm_pool_new(&bpid);
		if (ret)
			return NULL;
#else
		pr_err("No dynamic BPID allocator available\n");
		return NULL;
#endif
	} else
		bpid = params->bpid;
#ifdef CONFIG_FSL_BMAN_CONFIG
	if (params->flags & BMAN_POOL_FLAG_THRESH) {
		int ret;
		BUG_ON(!(params->flags & BMAN_POOL_FLAG_DYNAMIC_BPID));
		ret = bm_pool_set(bpid, params->thresholds);
		if (ret)
			goto err;
	} else
		/* ignore result, if it fails, there was no CCSR */
		bm_pool_set(bpid, zero_thresholds);
#else
	if (params->flags & BMAN_POOL_FLAG_THRESH)
		goto err;
#endif
	pool = kmalloc(sizeof(*pool), GFP_KERNEL);
	if (!pool)
		goto err;
	pool->sp = NULL;
	pool->sp_fill = 0;
	pool->params = *params;
	if (params->flags & BMAN_POOL_FLAG_DYNAMIC_BPID)
		pool->params.bpid = bpid;
	if (params->flags & BMAN_POOL_FLAG_STOCKPILE) {
		pool->sp = kmalloc(sizeof(struct bm_buffer) * BMAN_STOCKPILE_SZ,
					GFP_KERNEL);
		if (!pool->sp)
			goto err;
	}
	if (pool->params.flags & BMAN_POOL_FLAG_DEPLETION) {
		struct bman_portal *p = get_affine_portal();
		if (!p->pools || !bman_depletion_get(&p->pools[0], bpid)) {
			pr_err("Depletion events disabled for bpid %d\n", bpid);
			goto err;
		}
		depletion_link(p, pool);
		put_affine_portal();
	}
	return pool;
err:
#ifdef CONFIG_FSL_BMAN_CONFIG
	if (params->flags & BMAN_POOL_FLAG_THRESH)
		bm_pool_set(bpid, zero_thresholds);
	if (params->flags & BMAN_POOL_FLAG_DYNAMIC_BPID)
		bm_pool_free(bpid);
#endif
	if (pool) {
		if (pool->sp)
			kfree(pool->sp);
		kfree(pool);
	}
	return NULL;
}
EXPORT_SYMBOL(bman_new_pool);

void bman_free_pool(struct bman_pool *pool)
{
#ifdef CONFIG_FSL_BMAN_CONFIG
	if (pool->params.flags & BMAN_POOL_FLAG_THRESH)
		bm_pool_set(pool->params.bpid, zero_thresholds);
#endif
	if (pool->params.flags & BMAN_POOL_FLAG_DEPLETION)
		depletion_unlink(pool);
#ifdef CONFIG_FSL_BMAN_CONFIG
	if (pool->params.flags & BMAN_POOL_FLAG_DYNAMIC_BPID)
		bm_pool_free(pool->params.bpid);
#endif
	kfree(pool);
}
EXPORT_SYMBOL(bman_free_pool);

const struct bman_pool_params *bman_get_params(const struct bman_pool *pool)
{
	return &pool->params;
}
EXPORT_SYMBOL(bman_get_params);

static inline void rel_set_thresh(struct bman_portal *p, int check)
{
	if (!check || !bm_rcr_get_ithresh(p->p))
		bm_rcr_set_ithresh(p->p, RCR_ITHRESH);
}

/* Used as a wait_event() expression. If it returns non-NULL, any lock will
 * remain held. */
static struct bm_rcr_entry *try_rel_start(struct bman_portal **p)
{
	struct bm_rcr_entry *r;
	*p = get_affine_portal();
	if (unlikely(!*p)) {
		put_affine_portal();
		return NULL;
	}
	local_irq_disable();
	if (bm_rcr_get_avail((*p)->p) < RCR_THRESH)
		bm_rcr_cci_update((*p)->p);
	r = bm_rcr_start((*p)->p);
	if (unlikely(!r)) {
		rel_set_thresh(*p, 1);
		local_irq_enable();
		put_affine_portal();
	}
	return r;
}

static inline int wait_rel_start(struct bman_portal **p,
			struct bm_rcr_entry **rel, u32 flags)
{
	int ret = 0;
	if (flags & BMAN_RELEASE_FLAG_WAIT_INT)
		ret = wait_event_interruptible(affine_queue,
				(*rel = try_rel_start(p)));
	else
		wait_event(affine_queue, (*rel = try_rel_start(p)));
	return ret;
}

/* This copies Qman's eqcr_completed() routine, see that for details */
static int rel_completed(struct bman_portal *p, u32 rcr_poll)
{
	u32 tr_cons = p->rcr_cons;
	if (rcr_poll & 0xc0000000) {
		rcr_poll &= 0x7fffffff;
		tr_cons ^= 0x80000000;
	}
	if (tr_cons >= rcr_poll)
		return 1;
	if ((rcr_poll - tr_cons) > BM_RCR_SIZE)
		return 1;
	if (!bm_rcr_get_fill(p->p))
		/* If RCR is empty, we must have completed */
		return 1;
	rel_set_thresh(p, 0);
	return 0;
}

static inline void rel_commit(struct bman_portal *p, u32 flags, u8 num)
{
	u32 rcr_poll;
	bm_rcr_pvb_commit(p->p, BM_RCR_VERB_CMD_BPID_SINGLE |
			(num & BM_RCR_VERB_BUFCOUNT_MASK));
	/* increment the producer count and capture it for SYNC */
	rcr_poll = ++p->rcr_prod;
	if ((flags & BMAN_RELEASE_FLAG_WAIT_SYNC) ==
			BMAN_RELEASE_FLAG_WAIT_SYNC)
		rel_set_thresh(p, 1);
	local_irq_enable();
	put_affine_portal();
	if ((flags & BMAN_RELEASE_FLAG_WAIT_SYNC) !=
			BMAN_RELEASE_FLAG_WAIT_SYNC)
		return;
	/* So we're supposed to wait until the commit is consumed */
	if (flags & BMAN_RELEASE_FLAG_WAIT_INT)
		/* See bman_release() as to why we're ignoring return codes
		 * from wait_***(). */
		wait_event_interruptible(affine_queue,
					rel_completed(p, rcr_poll));
	else
		wait_event(affine_queue, rel_completed(p, rcr_poll));
}

static inline int __bman_release(struct bman_pool *pool,
			const struct bm_buffer *bufs, u8 num, u32 flags)
{
	struct bman_portal *p;
	struct bm_rcr_entry *r;
	u8 i;

	/* FIXME: I'm ignoring BMAN_PORTAL_FLAG_COMPACT for now. */
	r = try_rel_start(&p);
	if (unlikely(!r)) {
		if (flags & BMAN_RELEASE_FLAG_WAIT) {
			int ret = wait_rel_start(&p, &r, flags);
			if (ret)
				return ret;
		} else
			return -EBUSY;
		BM_ASSERT(r != NULL);
	}
	r->bpid = pool->params.bpid;
	for (i = 0; i < num; i++) {
		r->bufs[i].hi = bufs[i].hi;
		r->bufs[i].lo = bufs[i].lo;
	}
	/* Issue the release command and wait for sync if requested. NB: the
	 * commit can't fail, only waiting can. Don't propogate any failure if a
	 * signal arrives, otherwise the caller can't distinguish whether the
	 * release was issued or not. Code for user-space can check
	 * signal_pending() after we return. */
	rel_commit(p, flags, num);
	return 0;
}

int bman_release(struct bman_pool *pool, const struct bm_buffer *bufs, u8 num,
			u32 flags)
{
#ifdef CONFIG_FSL_BMAN_CHECKING
	if (!num || (num > 8))
		return -EINVAL;
	if (pool->params.flags & BMAN_POOL_FLAG_NO_RELEASE)
		return -EINVAL;
#endif
	/* Without stockpile, this API is a pass-through to the h/w operation */
	if (!(pool->params.flags & BMAN_POOL_FLAG_STOCKPILE))
		return __bman_release(pool, bufs, num, flags);
	/* This needs some explanation. Adding the given buffers may take the
	 * stockpile over the threshold, but in fact the stockpile may already
	 * *be* over the threshold if a previous release-to-hw attempt had
	 * failed. So we have 3 cases to cover;
	 *   1. we add to the stockpile and don't hit the threshold,
	 *   2. we add to the stockpile, hit the threshold and release-to-hw,
	 *   3. we have to release-to-hw before adding to the stockpile
	 *      (not enough room in the stockpile for case 2).
	 * Our constraints on thresholds guarantee that in case 3, there must be
	 * at least 8 bufs already in the stockpile, so all release-to-hw ops
	 * are for 8 bufs. Despite all this, the API must indicate whether the
	 * given buffers were taken off the caller's hands, irrespective of
	 * whether a release-to-hw was attempted. */
	while (num) {
		/* Add buffers to stockpile if they fit */
		if ((pool->sp_fill + num) < BMAN_STOCKPILE_SZ) {
			memcpy(pool->sp + pool->sp_fill, bufs,
				sizeof(struct bm_buffer) * num);
			pool->sp_fill += num;
			num = 0; /* --> will return success no matter what */
		}
		/* Do hw op if hitting the high-water threshold */
		if ((pool->sp_fill + num) >= BMAN_STOCKPILE_HIGH) {
			u8 ret = __bman_release(pool,
				pool->sp + (pool->sp_fill - 8), 8, flags);
			if (ret)
				return (num ? ret : 0);
			pool->sp_fill -= 8;
		}
	}
	return 0;
}
EXPORT_SYMBOL(bman_release);

static inline int __bman_acquire(struct bman_pool *pool, struct bm_buffer *bufs,
					u8 num)
{
	struct bman_portal *p = get_affine_portal();
	struct bm_mc_command *mcc;
	struct bm_mc_result *mcr;
	u8 ret;

	local_irq_disable();
	mcc = bm_mc_start(p->p);
	mcc->acquire.bpid = pool->params.bpid;
	bm_mc_commit(p->p, BM_MCC_VERB_CMD_ACQUIRE |
			(num & BM_MCC_VERB_ACQUIRE_BUFCOUNT));
	while (!(mcr = bm_mc_result(p->p)))
		cpu_relax();
	ret = num = mcr->verb & BM_MCR_VERB_ACQUIRE_BUFCOUNT;
	while (num--) {
		bufs[num].bpid = pool->params.bpid;
		bufs[num].hi = mcr->acquire.bufs[num].hi;
		bufs[num].lo = mcr->acquire.bufs[num].lo;
	}
	local_irq_enable();
	put_affine_portal();
	return ret;
}

int bman_acquire(struct bman_pool *pool, struct bm_buffer *bufs, u8 num,
			u32 flags)
{
#ifdef CONFIG_FSL_BMAN_CHECKING
	if (!num || (num > 8))
		return -EINVAL;
	if (pool->params.flags & BMAN_POOL_FLAG_ONLY_RELEASE)
		return -EINVAL;
#endif
	/* Without stockpile, this API is a pass-through to the h/w operation */
	if (!(pool->params.flags & BMAN_POOL_FLAG_STOCKPILE))
		return __bman_acquire(pool, bufs, num);
#ifdef CONFIG_SMP
	panic("Bman stockpiles are not SMP-safe!");
#endif
	/* Only need a h/w op if we'll hit the low-water thresh */
	if (!(flags & BMAN_ACQUIRE_FLAG_STOCKPILE) &&
			(pool->sp_fill <= (BMAN_STOCKPILE_LOW + num))) {
		u8 ret = __bman_acquire(pool, pool->sp + pool->sp_fill, 8);
		if (!ret)
			return -ENOMEM;
		BUG_ON(ret != 8);
		pool->sp_fill += 8;
	} else if (pool->sp_fill < num)
		return -ENOMEM;
	memcpy(bufs, pool->sp + (pool->sp_fill - num),
		sizeof(struct bm_buffer) * num);
	pool->sp_fill -= num;
	return num;
}
EXPORT_SYMBOL(bman_acquire);

