/* Copyright (c) 2008, 2009, Freescale Semiconductor, Inc.
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

#include "pme2_private.h"

/* The pme_ctx state machine is described via the following list of
 * internal PME_CTX_FLAG_*** bits and cross-referenced to the APIs (and
 * functionality) they track.
 *
 * DEAD: set at any point, an error has been hit, doesn't "cause" disabling or
 * any autonomous ref-decrement (been there, hit the gotchas, won't do it
 * again).
 *
 * DISABLING: set by pme_ctx_disable() at any point that is not already
 * disabling, disabled, or in ctrl, and the ref is decremented. DISABLING is
 * unset by pme_ctx_enable().
 *
 * DISABLED: once pme_ctx_disable() has set DISABLING and refs==0, DISABLED is
 * set before returning. (Any failure will clear DISABLING and increment the ref
 * count.) DISABLING is unset by pme_ctx_enable().
 *
 * ENABLING: set by pme_ctx_enable() provided the context is disabled, not dead,
 * not in RECONFIG, and not already enabling. Once set, the ref is incremented
 * and the tx FQ is scheduled (for non-exclusive flows). If this fails, the ref
 * is decremented and the context is re-disabled. ENABLING is unset once
 * pme_ctx_enable() completes.
 *
 * RECONFIG: set by pme_ctx_reconfigure_[rt]x() provided the context is
 * disabled, not dead, and not already in reconfig. RECONFIG is cleared prior to
 * the function returning.
 *
 * Simplifications: the do_flag() wrapper provides synchronised modifications of
 * the ctx 'flags', and callers can rely on the following implications to reduce
 * the number of flags in the masks being passed in;
 * 	DISABLED implies DISABLING (and enable will clear both)
 */

#define PME_CTX_FLAG_DEAD        0x80000000
#define PME_CTX_FLAG_DISABLING   0x40000000
#define PME_CTX_FLAG_DISABLED    0x20000000
#define PME_CTX_FLAG_ENABLING    0x10000000
#define PME_CTX_FLAG_RECONFIG    0x01000000

#define PME_CTX_FLAG_PRIVATE     0xff000000

/* This wrapper simplifies conditional (and locked) read-modify-writes to
 * 'flags'. Inlining should allow the compiler to optimise it based on the
 * parameters, eg. if 'must_be_set'/'must_not_be_set' are zero it will
 * degenerate to an unconditional read-modify-write, if 'to_set'/'to_unset' are
 * zero it will degenerate to a read-only flag-check, etc. */
static inline int do_flags(struct pme_ctx *ctx,
			u32 must_be_set, u32 must_not_be_set,
			u32 to_set, u32 to_unset)
{
	int err = -EBUSY;
	spin_lock_irq(&ctx->lock);
	if (((ctx->flags & must_be_set) == must_be_set) &&
			!(ctx->flags & must_not_be_set)) {
		ctx->flags |= to_set;
		ctx->flags &= ~to_unset;
		err = 0;
	}
	spin_unlock_irq(&ctx->lock);
	return err;
}

static enum qman_cb_dqrr_result cb_dqrr(struct qman_portal *, struct qman_fq *,
				const struct qm_dqrr_entry *);
static void cb_ern(struct qman_portal *, struct qman_fq *,
				const struct qm_mr_entry *);
static void cb_dc_ern(struct qman_portal *, struct qman_fq *,
				const struct qm_mr_entry *);
static void cb_fqs(struct qman_portal *, struct qman_fq *,
				const struct qm_mr_entry *);
static const struct qman_fq_cb pme_fq_base_in = {
	.fqs = cb_fqs
};
static const struct qman_fq_cb pme_fq_base_out = {
	.dqrr = cb_dqrr,
	.ern = cb_ern,
	.dc_ern = cb_dc_ern,
	.fqs = cb_fqs
};

/* Globals related to competition for PME_EFQC, ie. exclusivity */
static DECLARE_WAIT_QUEUE_HEAD(exclusive_queue);
static spinlock_t exclusive_lock = SPIN_LOCK_UNLOCKED;
static unsigned int exclusive_refs;
static struct pme_ctx *exclusive_ctx;

/* Index 0..255, bools do indicated which errors are serious
 * 0x40, 0x41, 0x48, 0x49, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x59, 0x5a, 0x5b,
 * 0x5c, 0x5d, 0x5f, 0x60, 0x80, 0xc0, 0xc1, 0xc2, 0xc4, 0xd2,
 * 0xd4, 0xd5, 0xd7, 0xd9, 0xda, 0xe0, 0xe7
 */
static u8 serious_error_vec[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* TODO: this is hitting the rx FQ with a large blunt instrument, ie. park()
 * does a retire, query, oos, and (re)init. It's possible to force-eligible the
 * rx FQ instead, then use a DCA_PK within the cb_dqrr() callback to park it.
 * Implement this optimisation later if it's an issue (and incur the additional
 * complexity in the state-machine). */
static int park(struct qman_fq *fq, struct qm_mcc_initfq *initfq)
{
	int ret;
	u32 flags;

	ret = qman_retire_fq(fq, &flags);
	if (ret)
		return ret;
	BUG_ON(flags & QMAN_FQ_STATE_BLOCKOOS);
	/* We can't revert from now on */
	ret = qman_query_fq(fq, &initfq->fqd);
	BUG_ON(ret);
	ret = qman_oos_fq(fq);
	BUG_ON(ret);
	initfq->we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_CONTEXTA |
			QM_INITFQ_WE_CONTEXTA;
	ret = qman_init_fq(fq, 0, initfq);
	BUG_ON(ret);
	return 0;
}

static inline int reconfigure_rx(struct pme_ctx *ctx, int to_park, u8 qosout,
				enum qm_channel dest,
				const struct qm_fqd_stashing *stashing)
{
	struct qm_mcc_initfq initfq;
	u32 flags = QMAN_INITFQ_FLAG_SCHED;
	int ret;

	ret = do_flags(ctx, PME_CTX_FLAG_DISABLED,
			PME_CTX_FLAG_DEAD | PME_CTX_FLAG_RECONFIG,
			PME_CTX_FLAG_RECONFIG, 0);
	if (ret)
		return ret;
	if (to_park) {
		ret = park(&ctx->fq, &initfq);
		if (ret)
			goto done;
	}
	initfq.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_FQCTRL;
	initfq.fqd.dest.wq = qosout;
	if (stashing) {
		initfq.we_mask |= QM_INITFQ_WE_CONTEXTA;
		initfq.fqd.context_a.stashing = *stashing;
		initfq.fqd.fq_ctrl = QM_FQCTRL_CTXASTASHING;
	} else
		initfq.fqd.fq_ctrl = 0; /* disable stashing */
	if (ctx->flags & PME_CTX_FLAG_LOCAL)
		flags |= QMAN_INITFQ_FLAG_LOCAL;
	else {
		initfq.fqd.dest.channel = dest;
		/* Set hold-active *IFF* it's a pool channel */
		if (dest >= qm_channel_pool1)
			initfq.fqd.fq_ctrl |= QM_FQCTRL_HOLDACTIVE;
	}
	ret = qman_init_fq(&ctx->fq, flags, &initfq);
done:
	do_flags(ctx, 0, 0, 0, PME_CTX_FLAG_RECONFIG);
	return ret;
}

/* this code is factored out of pme_ctx_disable() and get_ctrl() */
static int empty_pipeline(struct pme_ctx *ctx, u32 flags)
{
	int ret;
	if (flags & PME_CTX_OP_WAIT) {
		if (flags & PME_CTX_OP_WAIT_INT) {
			ret = -EINTR;
			wait_event_interruptible(ctx->queue,
				!(ret = atomic_read(&ctx->refs)));
		} else
			wait_event(ctx->queue,
				!(ret = atomic_read(&ctx->refs)));
	} else
		ret = atomic_read(&ctx->refs);
	if (ret)
		/* convert a +ve ref-count to a -ve error code */
		ret = -EBUSY;
	return ret;
}

int pme_ctx_init(struct pme_ctx *ctx, u32 flags, u32 bpid, u8 qosin,
			u8 qosout, enum qm_channel dest,
			const struct qm_fqd_stashing *stashing)
{
	u32 fqid_rx, fqid_tx;
	int rxinit = 0, ret = -ENOMEM, fqin_inited = 0;

	ctx->fq.cb = pme_fq_base_out;
	atomic_set(&ctx->refs, 0);
	ctx->flags = (flags & ~PME_CTX_FLAG_PRIVATE) | PME_CTX_FLAG_DISABLED |
			PME_CTX_FLAG_DISABLING;
	spin_lock_init(&ctx->lock);
	init_waitqueue_head(&ctx->queue);
	INIT_LIST_HEAD(&ctx->tokens);
	ctx->seq_num = 0;
	ctx->fqin = NULL;
	ctx->hw_flow = NULL;
	ctx->hw_residue = NULL;

	fqid_rx = qm_fq_new();
	fqid_tx = qm_fq_new();
	ctx->fqin = slabfq_alloc();
	if (!fqid_rx || !fqid_tx || !ctx->fqin)
		goto err;
	ctx->fqin->cb = pme_fq_base_in;
	if (qman_create_fq(fqid_rx, QMAN_FQ_FLAG_TO_DCPORTAL |
			((flags & PME_CTX_FLAG_LOCKED) ?
				QMAN_FQ_FLAG_LOCKED : 0), ctx->fqin))
		goto err;
	fqin_inited = 1;
	if (qman_create_fq(fqid_tx, QMAN_FQ_FLAG_NO_ENQUEUE |
			((flags & PME_CTX_FLAG_LOCKED) ?
				QMAN_FQ_FLAG_LOCKED : 0), &ctx->fq))
		goto err;
	rxinit = 1;
	/* Input FQ */
	if (!(flags & PME_CTX_FLAG_DIRECT)) {
		ctx->hw_flow = pme_hw_flow_new();
		if (!ctx->hw_flow)
			goto err;
	}
	ret = pme_ctx_reconfigure_tx(ctx, bpid, qosin);
	if (ret)
		goto err;
	/* Output FQ */
	ret = reconfigure_rx(ctx, 0, qosout, dest, stashing);
	if (ret) {
		/* Need to OOS the FQ before it gets free'd */
		ret = qman_oos_fq(ctx->fqin);
		BUG_ON(ret);
		goto err;
	}
	return 0;
err:
	if (fqid_rx)
		qm_fq_free(fqid_rx);
	if (fqid_tx)
		qm_fq_free(fqid_tx);
	if (ctx->hw_flow)
		pme_hw_flow_free(ctx->hw_flow);
	if (ctx->fqin) {
		if (fqin_inited)
			qman_destroy_fq(ctx->fqin, 0);
		slabfq_free(ctx->fqin);
	}
	if (rxinit)
		qman_destroy_fq(&ctx->fq, 0);
	return ret;
}
EXPORT_SYMBOL(pme_ctx_init);

/* NB, we don't lock here because there must be no other callers (even if we
 * locked, what does the loser do after we win?) */
void pme_ctx_finish(struct pme_ctx *ctx)
{
	u32 flags, fqid_rx, fqid_tx;
	int ret;

	ret = do_flags(ctx, PME_CTX_FLAG_DISABLED, PME_CTX_FLAG_RECONFIG, 0, 0);
	BUG_ON(ret);
	/* Rx/Tx are empty (coz ctx is disabled) so retirement should be
	 * immediate */
	ret = qman_retire_fq(ctx->fqin, &flags);
	BUG_ON(ret);
	BUG_ON(flags & QMAN_FQ_STATE_BLOCKOOS);
	ret = qman_retire_fq(&ctx->fq, &flags);
	BUG_ON(ret);
	BUG_ON(flags & QMAN_FQ_STATE_BLOCKOOS);
	/* OOS and free (don't kfree fq, it's a static ctx member) */
	ret = qman_oos_fq(ctx->fqin);
	BUG_ON(ret);
	ret = qman_oos_fq(&ctx->fq);
	BUG_ON(ret);
	fqid_rx = qman_fq_fqid(ctx->fqin);
	fqid_tx = qman_fq_fqid(&ctx->fq);
	qman_destroy_fq(ctx->fqin, 0);
	qman_destroy_fq(&ctx->fq, 0);
	qm_fq_free(fqid_rx);
	qm_fq_free(fqid_tx);
	slabfq_free(ctx->fqin); /* the fq was dynamically allocated */
	if (ctx->hw_flow)
		pme_hw_flow_free(ctx->hw_flow);
	if (ctx->hw_residue)
		pme_hw_residue_free(ctx->hw_residue);
}
EXPORT_SYMBOL(pme_ctx_finish);

int pme_ctx_is_disabled(struct pme_ctx *ctx)
{
	return (ctx->flags & PME_CTX_FLAG_DISABLED);
}
EXPORT_SYMBOL(pme_ctx_is_disabled);

int pme_ctx_is_dead(struct pme_ctx *ctx)
{
	return (ctx->flags & PME_CTX_FLAG_DEAD);
}
EXPORT_SYMBOL(pme_ctx_is_dead);

int pme_ctx_disable(struct pme_ctx *ctx, u32 flags)
{
	struct qm_mcc_initfq initfq;
	int ret;

	ret = do_flags(ctx, 0, PME_CTX_FLAG_DISABLING, PME_CTX_FLAG_DISABLING,
			0);
	if (ret)
		return ret;
	/* Make sure the pipeline is empty */
	atomic_dec(&ctx->refs);
	ret = empty_pipeline(ctx, flags);
	if (!ret && !(ctx->flags & PME_CTX_FLAG_EXCLUSIVE))
		/* Park fqin (exclusive is always parked) */
		ret = park(ctx->fqin, &initfq);
	if (ret) {
		atomic_inc(&ctx->refs);
		do_flags(ctx, 0, 0, 0, PME_CTX_FLAG_DISABLING);
		wake_up(&ctx->queue);
		return ret;
	}
	/* Our ORP got reset too, so reset the sequence number */
	ctx->seq_num = 0;
	do_flags(ctx, 0, 0, PME_CTX_FLAG_DISABLED, 0);
	return 0;
}
EXPORT_SYMBOL(pme_ctx_disable);

int pme_ctx_enable(struct pme_ctx *ctx)
{
	int ret;
	ret = do_flags(ctx, PME_CTX_FLAG_DISABLED,
			PME_CTX_FLAG_DEAD | PME_CTX_FLAG_RECONFIG |
			PME_CTX_FLAG_ENABLING,
			PME_CTX_FLAG_ENABLING, 0);
	if (ret)
		return ret;
	if (!(ctx->flags & PME_CTX_FLAG_EXCLUSIVE)) {
		ret = qman_init_fq(ctx->fqin, QMAN_INITFQ_FLAG_SCHED, NULL);
		if (ret) {
			do_flags(ctx, 0, 0, 0, PME_CTX_FLAG_ENABLING);
			return ret;
		}
	}
	atomic_inc(&ctx->refs);
	do_flags(ctx, 0, 0, 0, PME_CTX_FLAG_DISABLED | PME_CTX_FLAG_DISABLING |
				PME_CTX_FLAG_ENABLING);
	return 0;
}
EXPORT_SYMBOL(pme_ctx_enable);

int pme_ctx_reconfigure_tx(struct pme_ctx *ctx, u32 bpid, u8 qosin)
{
	struct qm_mcc_initfq initfq;
	int ret;

	ret = do_flags(ctx, PME_CTX_FLAG_DISABLED,
			PME_CTX_FLAG_DEAD | PME_CTX_FLAG_RECONFIG,
			PME_CTX_FLAG_RECONFIG, 0);
	if (ret)
		return ret;
	memset(&initfq,0,sizeof(initfq));
	pme_initfq(&initfq, ctx->hw_flow, qosin, bpid, qman_fq_fqid(&ctx->fq));
	if (!(ctx->flags & PME_CTX_FLAG_NO_ORP)) {
		initfq.we_mask |= QM_INITFQ_WE_FQCTRL | QM_INITFQ_WE_ORPC;
		/* ORPRWS==1 means the ORP window is max 64 frames. Given that
		 * the out-of-order problem is (usually?) limited to spraying
		 * over different EQCRs from different cores, it shouldn't be
		 * possible to get more than this far behind (8 full EQCRs is 56
		 * frames). */
		initfq.fqd.orprws = 1;
		initfq.fqd.oa = 0;
		initfq.fqd.olws = 0;
		initfq.fqd.fq_ctrl |= QM_FQCTRL_ORP;
	}
	ret = qman_init_fq(ctx->fqin, 0, &initfq);
	do_flags(ctx, 0, 0, 0, PME_CTX_FLAG_RECONFIG);
	return ret;
}
EXPORT_SYMBOL(pme_ctx_reconfigure_tx);

int pme_ctx_reconfigure_rx(struct pme_ctx *ctx, u8 qosout,
		enum qm_channel dest, const struct qm_fqd_stashing *stashing)
{
	return reconfigure_rx(ctx, 1, qosout, dest, stashing);
}
EXPORT_SYMBOL(pme_ctx_reconfigure_rx);

/* Helpers for 'ctrl' and 'work' APIs. These are used when the 'ctx' in question
 * is EXCLUSIVE. */
static inline void release_exclusive(struct pme_ctx *ctx)
{
	BUG_ON(exclusive_ctx != ctx);
	BUG_ON(!exclusive_refs);
	spin_lock_irq(&exclusive_lock);
	if (!(--exclusive_refs)) {
		exclusive_ctx = NULL;
		pme2_exclusive_unset();
		wake_up(&exclusive_queue);
	}
	spin_unlock_irq(&exclusive_lock);
}
static int __try_exclusive(struct pme_ctx *ctx)
{
	int ret = 0;
	spin_lock_irq(&exclusive_lock);
	if (exclusive_refs) {
		/* exclusivity already held, continue if we're the owner */
		if (exclusive_ctx != ctx)
			ret = -EBUSY;
	} else {
		/* it's not currently held */
		ret = pme2_exclusive_set(ctx->fqin);
		if (!ret)
			exclusive_ctx = ctx;
	}
	if (!ret)
		exclusive_refs++;
	spin_unlock_irq(&exclusive_lock);
	return ret;
}
/* Use this macro as the wait expression because we don't want to continue
 * looping if the reason we're failing is that we don't have CCSR access
 * (-ENODEV). */
#define try_exclusive(ret, ctx) \
	(!(ret = __try_exclusive(ctx)) || (ret == -ENODEV))
static inline int get_exclusive(struct pme_ctx *ctx, u32 flags)
{
	int ret;
	if (flags & PME_CTX_OP_WAIT) {
		if (flags & PME_CTX_OP_WAIT_INT) {
			ret = -EINTR;
			wait_event_interruptible(exclusive_queue,
					try_exclusive(ret, ctx));
		} else
			wait_event(exclusive_queue,
					try_exclusive(ret, ctx));
	} else
		ret = __try_exclusive(ctx);
	return ret;
}
/* Used for 'work' APIs, convert PME->QMAN wait flags. The PME and
 * QMAN "wait" flags have been aligned so that the below conversion should
 * compile with good straight-line speed. */
static inline u32 ctrl2eq(u32 flags)
{
	return flags & (QMAN_ENQUEUE_FLAG_WAIT | QMAN_ENQUEUE_FLAG_WAIT_INT);
}

static inline void release_work(struct pme_ctx *ctx)
{
	if (atomic_dec_and_test(&ctx->refs))
		wake_up(&ctx->queue);
}

static int __try_work(struct pme_ctx *ctx)
{
	atomic_inc(&ctx->refs);
	if (unlikely(ctx->flags & (PME_CTX_FLAG_DEAD |
			PME_CTX_FLAG_DISABLING))) {
		release_work(ctx);
		if (ctx->flags & (PME_CTX_FLAG_DEAD | PME_CTX_FLAG_DISABLING))
			return -EIO;
		return -EAGAIN;
	}
	return 0;
}
/* Exact same comment as for try_ctrl() */
#define try_work(ret, ctx) \
	(!(ret = __try_work(ctx)) || (ctx->flags & (PME_CTX_FLAG_DEAD | \
					PME_CTX_FLAG_DISABLING)))
static int get_work(struct pme_ctx *ctx, u32 flags)
{
	int ret;
	if (flags & PME_CTX_OP_WAIT) {
		if (flags & PME_CTX_OP_WAIT_INT) {
			ret = -EINTR;
			wait_event_interruptible(ctx->queue,
					try_work(ret, ctx));
		} else
			wait_event(ctx->queue, try_work(ret, ctx));
	} else
		ret = __try_work(ctx);
	return ret;
}

static int do_work(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd,
		struct pme_ctx_token *token)
{
	u32 seq_num;
	int ret = get_work(ctx, flags);
	if (ret)
		return ret;
	if (ctx->flags & PME_CTX_FLAG_EXCLUSIVE) {
		ret = get_exclusive(ctx, flags);
		if (ret) {
			release_work(ctx);
			return ret;
		}
	}
	BUG_ON(sizeof(*fd) != sizeof(token->blob));
	memcpy(&token->blob, fd, sizeof(*fd));
	spin_lock_irq(&ctx->lock);
	seq_num = ctx->seq_num++;
	ctx->seq_num &= QM_EQCR_SEQNUM_SEQMASK; /* rollover at 2^14 */
	list_add_tail(&token->node, &ctx->tokens);
	spin_unlock_irq(&ctx->lock);
	if (ctx->flags & PME_CTX_FLAG_NO_ORP)
		ret = qman_enqueue(ctx->fqin, fd, ctrl2eq(flags));
	else
		ret = qman_enqueue_orp(ctx->fqin, fd, ctrl2eq(flags),
					ctx->fqin, seq_num);
	if (ret) {
		if (ctx->flags & PME_CTX_FLAG_EXCLUSIVE)
			release_exclusive(ctx);
		release_work(ctx);
	}
	return ret;
}

int pme_ctx_ctrl_update_flow(struct pme_ctx *ctx, u32 flags,
		struct pme_flow *params,  struct pme_ctx_ctrl_token *token)
{
	struct qm_fd fd;

	BUG_ON(ctx->flags & PME_CTX_FLAG_DIRECT);
	token->base_token.cmd_type = pme_cmd_flow_write;

	if (flags & PME_CTX_OP_RESETRESLEN) {
		if (ctx->hw_residue) {
			params->ren = 1;
			flags |= PME_CMD_FCW_RES;
		} else
			flags &= ~PME_CMD_FCW_RES;
	}
	/* allocate residue memory if it is being added */
	if ((flags & PME_CMD_FCW_RES) && params->ren && !ctx->hw_residue) {
		ctx->hw_residue = pme_hw_residue_new();
		if (!ctx->hw_residue)
			return -ENOMEM;
	}
	/* enqueue the FCW command to PME */
	memset(&fd, 0, sizeof(fd));
	token->internal_flow_ptr = pme_hw_flow_new();
	memcpy(token->internal_flow_ptr, params, sizeof(struct pme_flow));
	pme_fd_cmd_fcw(&fd, flags & PME_CMD_FCW_ALL,
			(struct pme_flow *)token->internal_flow_ptr,
			ctx->hw_residue);
	return do_work(ctx, flags, &fd, &token->base_token);
}
EXPORT_SYMBOL(pme_ctx_ctrl_update_flow);

int pme_ctx_ctrl_read_flow(struct pme_ctx *ctx, u32 flags,
		struct pme_flow *params, struct pme_ctx_ctrl_token *token)
{
	struct qm_fd fd;

	BUG_ON(ctx->flags & (PME_CTX_FLAG_DIRECT | PME_CTX_FLAG_PMTCC));
	token->base_token.cmd_type = pme_cmd_flow_read;
	/* enqueue the FCR command to PME */
	token->usr_flow_ptr = params;
	token->internal_flow_ptr = pme_hw_flow_new();
	memset(&fd, 0, sizeof(fd));
	pme_fd_cmd_fcr(&fd, (struct pme_flow *)token->internal_flow_ptr);
	return do_work(ctx, flags, &fd, &token->base_token);
}
EXPORT_SYMBOL(pme_ctx_ctrl_read_flow);

int pme_ctx_ctrl_nop(struct pme_ctx *ctx, u32 flags,
		struct pme_ctx_ctrl_token *token)
{
	struct qm_fd fd;

	token->base_token.cmd_type = pme_cmd_nop;
	/* enqueue the NOP command to PME */
	memset(&fd, 0, sizeof(fd));
	fd.addr_hi = 0xfeed;
	fd.addr_lo = (u32)token;
	pme_fd_cmd_nop(&fd);
	return do_work(ctx, flags, &fd, &token->base_token);
}
EXPORT_SYMBOL(pme_ctx_ctrl_nop);

int pme_ctx_scan(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd, u32 args,
		struct pme_ctx_token *token)
{
	BUG_ON(ctx->flags & PME_CTX_FLAG_PMTCC);
	token->cmd_type = pme_cmd_scan;
	pme_fd_cmd_scan(fd, args);
	return do_work(ctx, flags, fd, token);
}
EXPORT_SYMBOL(pme_ctx_scan);

int pme_ctx_pmtcc(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd,
		struct pme_ctx_token *token)
{
	BUG_ON(!(ctx->flags & PME_CTX_FLAG_PMTCC));
	token->cmd_type = pme_cmd_pmtcc;
	pme_fd_cmd_pmtcc(fd);
	return do_work(ctx, flags, fd, token);
}
EXPORT_SYMBOL(pme_ctx_pmtcc);

int pme_ctx_exclusive_inc(struct pme_ctx *ctx, u32 flags)
{
	return get_exclusive(ctx, flags);
}
EXPORT_SYMBOL(pme_ctx_exclusive_inc);

void pme_ctx_exclusive_dec(struct pme_ctx *ctx)
{
	release_exclusive(ctx);
}
EXPORT_SYMBOL(pme_ctx_exclusive_dec);

/* The 99.99% case is that enqueues happen in order or they get order-restored
 * by the ORP, and so dequeues of responses happen in order too, so our FIFO
 * linked-list of tokens is append-on-enqueue and pop-on-dequeue, and all's
 * well.
 *
 * *EXCEPT*, if ever an enqueue gets rejected ... what then happens is that we
 * have dequeues and ERNs to deal with, and the order we see them in is not
 * necessarily the linked-list order. So we need to handle this in DQRR and MR
 * callbacks, without sacrificing fast-path performance. Ouch.
 *
 * We use pop_matching_token() to take care of the mess (inlined, of course). */
#define MATCH(fd1,fd2) \
	(((fd1)->addr_hi == (fd2)->addr_hi) && \
	((fd1)->addr_lo == (fd2)->addr_lo) && \
	((fd1)->opaque == (fd2)->opaque))
static inline struct pme_ctx_token *pop_matching_token(struct pme_ctx *ctx,
						const struct qm_fd *fd)
{
	struct pme_ctx_token *token;
	const struct qm_fd *t_fd;

	/* The fast-path case is that the for() loop actually degenerates into;
	 *     token = list_first_entry();
	 *     if (likely(MATCH()))
	 *         [done]
	 * The penalty of the slow-path case is the for() loop plus the fact
	 * we're optimising for a "likely" match first time, which might hurt
	 * when that assumption is wrong a few times in succession. */
	spin_lock_irq(&ctx->lock);
	list_for_each_entry(token, &ctx->tokens, node) {
		t_fd = (const struct qm_fd *)&token->blob[0];
		if (likely(MATCH(t_fd, fd))) {
			list_del(&token->node);
			goto found;
		}
	}
	token = NULL;
found:
	spin_unlock_irq(&ctx->lock);
	return token;
}

static inline void cb_helper(struct qman_portal *portal, struct pme_ctx *ctx,
				const struct qm_fd *fd, int error)
{
	struct pme_ctx_token *token;
	struct pme_ctx_ctrl_token *ctrl_token;
	/* Resist the urge to use "unlikely" - 'error' is a constant param to an
	 * inline fn, so the compiler can collapse this completely. */
	if (error)
		do_flags(ctx, 0, 0, PME_CTX_FLAG_DEAD, 0);
	token = pop_matching_token(ctx, fd);
	if (likely(token->cmd_type == pme_cmd_scan))
		ctx->cb(ctx, fd, token);
	else if (token->cmd_type == pme_cmd_pmtcc)
		ctx->cb(ctx, fd, token);
	else {
		/* outcast ctx and call supplied callback */
		ctrl_token = container_of(token, struct pme_ctx_ctrl_token,
					base_token);
		if (token->cmd_type == pme_cmd_flow_write) {
			/* Release the allocated flow context */
			pme_hw_flow_free(ctrl_token->internal_flow_ptr);
		} else if (token->cmd_type == pme_cmd_flow_read) {
			/* Copy read result */
			memcpy(ctrl_token->usr_flow_ptr,
				ctrl_token->internal_flow_ptr,
				sizeof(struct pme_flow));
			/* Release the allocated flow context */
			pme_hw_flow_free(ctrl_token->internal_flow_ptr);
		}
		ctrl_token->cb(ctx, fd, ctrl_token);
	}
	/* Consume the frame */
	if (ctx->flags & PME_CTX_FLAG_EXCLUSIVE)
		release_exclusive(ctx);
	if (atomic_dec_and_test(&ctx->refs))
		wake_up(&ctx->queue);
}

/* TODO: this scheme does not allow PME receivers to use held-active at all. Eg.
 * there's no configuration of held-active for 'fq', and if there was, there's
 * (a) nothing in the cb_dqrr() to support "park" or "defer" logic, and (b)
 * nothing in cb_fqs() to support a delayed FQPN (DCAP_PK) notification. */
static enum qman_cb_dqrr_result cb_dqrr(struct qman_portal *portal,
			struct qman_fq *fq, const struct qm_dqrr_entry *dq)
{
	u8 status = (u8)pme_fd_res_status(&dq->fd);
	u8 flags = pme_fd_res_flags(&dq->fd);
	struct pme_ctx *ctx = (struct pme_ctx *)fq;

	/* Put context into dead state is an unreliable or serious error is
	 * received
	 */
	if (unlikely(flags & PME_STATUS_UNRELIABLE))
		cb_helper(portal, ctx, &dq->fd, 1);
	else if (unlikely((serious_error_vec[status])))
		cb_helper(portal, ctx, &dq->fd, 1);
	else
		cb_helper(portal, ctx, &dq->fd, 0);

	return qman_cb_dqrr_consume;
}

static void cb_ern(struct qman_portal *portal, struct qman_fq *fq,
				const struct qm_mr_entry *mr)
{
	/* Give this the same handling as the error case through cb_dqrr(). */
	struct pme_ctx *ctx = (struct pme_ctx *)fq;
	cb_helper(portal, ctx, &mr->ern.fd, 1);
}

static void cb_dc_ern(struct qman_portal *portal, struct qman_fq *fq,
				const struct qm_mr_entry *mr)
{
	struct pme_ctx *ctx = (struct pme_ctx *)fq;
	/* This, umm, *shouldn't* happen. It's pretty bad. Things are expected
	 * to fall apart here, but we'll continue long enough to get out of
	 * interrupt context and let the user unwind whatever they can. */
	pr_err("PME2 h/w enqueue rejection - expect catastrophe!\n");
	cb_helper(portal, ctx, &mr->dcern.fd, 1);
}

static void cb_fqs(struct qman_portal *portal, struct qman_fq *fq,
				const struct qm_mr_entry *mr)
{
	u8 verb = mr->verb & QM_MR_VERB_TYPE_MASK;
	if (verb == QM_MR_VERB_FQRNI)
		return;
	/* nothing else is supposed to occur */
	BUG();
}
