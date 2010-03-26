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

#include "bman_test.h"

/* Assume this is broken, enable it afterwards */
#define BUG_NO_RCR_ITR

/* Test constants */
#define TPORTAL		0
#define TBUFS		10	/* Number released */
#define TRELEASEMAX	8	/* Maximum released at once */
#define TACQUIREMAX	8	/* Maximum acquired at once */
#define TDENTRY		2
#define TDEXIT		8
#define TADDRHI		0xabba
#define TADDRLO		0xdeadbeef

/* Global variables make life simpler */
static struct bm_portal *portal;
static struct bm_rcr_entry *rc;
static struct bm_mc_command *mcc;
static struct bm_mc_result *mcr;
static DEFINE_SPINLOCK(mc_lock);
static const u32 test_thresholds[4] = {TDENTRY, TDEXIT, 0, 0};
static DECLARE_WAIT_QUEUE_HEAD(queue);
static int isr_count;

/* Boolean switch for handling RCR_ITR */
#ifndef BUG_NO_RCR_ITR
static int rcr_thresh_on;
#endif

/* Bit-arrray representing releases to check against acquires. */
static u32 bufs[(TBUFS + 31) / 32];
static inline int BUFS_get(int idx)
{
	return (bufs[idx / 32] & (1 << (idx & 31))) ? 1 : 0;
}
static inline void BUFS_set(int idx)
{
	bufs[idx / 32] |= (1 << (idx & 31));
}
static inline void BUFS_unset(int idx)
{
	bufs[idx / 32] &= ~(u32)(1 << (idx & 31));
}

/* Helper for bm_mc_start() that checks the return code */
static void mc_start(void)
{
	mcc = bm_mc_start(portal);
	BUG_ON(!mcc);
}

/* Helper for bm_mc_result() that checks the response */
static void mc_commit(u8 verb)
{
	bm_mc_commit(portal, verb);
	do {
		mcr = bm_mc_result(portal);
	} while (!mcr);
	BUG_ON((mcr->verb & BM_MCR_VERB_CMD_MASK) !=
			(verb & BM_MCR_VERB_CMD_MASK));
}

/* Track RCR consumption */
static void rcr_update(void)
{
#ifndef BUG_NO_RCR_ITR
	u32 status;
	u8 fill;
	while ((fill = bm_rcr_get_fill(portal))) {
		pr_info("rcr_update: fill==%d\n", fill);
		bm_rcr_cci_update(portal);
	}
	status = bm_isr_status_read(portal);
	if (status & BM_PIRQ_RCRI) {
		BUG_ON(!rcr_thresh_on);
		bm_rcr_set_ithresh(portal, 0);
		rcr_thresh_on = 0;
		pr_info("Auto-update of RCR consumption\n");
		bm_isr_status_clear(portal, progress & BM_PIRQ_RCRI);
	}
#else
	bm_rcr_cci_update(portal);
#endif
}

/* Helper for bm_eqcr_start() that tracks ring consumption and checks the
 * return code */
static void rcr_start(void)
{
	/* If there are consumed RCR entries, track them now. The alternative
	 * is to catch an error in bm_rcr_start(), track consume entries then,
	 * and then retry bm_rcr_start(). */
	do {
		rcr_update();
		rc = bm_rcr_start(portal);
	} while (!rc);
}

/* Helper for bm_rcr_pvb_commit() */
static void rcr_commit(u8 numbufs)
{
	bm_rcr_pvb_commit(portal, BM_RCR_VERB_CMD_BPID_SINGLE | numbufs);
#ifndef BUG_NO_RCR_ITR
	if (!rcr_thresh_on && (bm_rcr_get_avail(portal) < 2)) {
		rcr_thresh_on = 1;
		bm_rcr_set_ithresh(portal, 1);
	}
#endif
}

static irqreturn_t portal_isr(int irq, void *ptr)
{
	pr_info("BMAN portal interrupt, isr_count=%d->%d\n", isr_count,
		isr_count + 1);
	isr_count++;
	bm_isr_inhibit(portal);
	wake_up(&queue);
	return IRQ_HANDLED;
}

void bman_test_low(struct bm_portal *__p)
{
	const struct bm_portal_config *config = bm_portal_config(__p);
	u32 bpid;
	int i, big_loop = 2;
	u32 status;
	int depleted = 1, last_count = 0;
#define WAIT_ISR() \
do { \
	last_count++; \
	wait_event(queue, isr_count == last_count); \
} while(0)

	portal = __p;

	i = bm_pool_new(&bpid);
	if (i)
		panic("can't allocate bpid");
	i = bm_pool_set(bpid, test_thresholds);
	if (i)
		panic("can't set thresholds");

	/*********************/
	/* Initialise portal */
	/*********************/
	if (bm_rcr_init(portal, bm_rcr_pvb, bm_rcr_cci) ||
			bm_mc_init(portal) || bm_isr_init(portal))
		panic("Portal setup failed");
	bm_isr_enable_write(portal, -1);
	bm_isr_disable_write(portal, 0);
	bm_isr_uninhibit(portal);
	bm_isr_status_clear(portal, 0xffffffff);

	pr_info("low-level test, start ccmode\n");

	if (request_irq(config->irq, portal_isr, 0, "Bman portal 0", NULL))
		panic("Can't register Bman portal 0 IRQ");
	/* Enable the BSCN mask bit corresponding to our bpid */
	bm_isr_bscn_mask(portal, bpid, 1);

	pr_info("Portal %d i/faces initialised\n", TPORTAL);

again:
	/* The portal's (interrupt) status register should be zero */
	status = bm_isr_status_read(portal);
	BUG_ON(status);

	/*************************/
	/* Release TBUFS buffers */
	/*************************/
	for (i = 0; i < TBUFS;) {
		int j = 0;
		rcr_start();
		rc->bpid = bpid;
		while ((j < TRELEASEMAX) && (i < TBUFS)) {
			rc->bufs[j].hi = TADDRHI;
			rc->bufs[j].lo = TADDRLO + i;
			BUFS_set(i);
			i++;
			j++;
		}
		rcr_commit(j);
		if (depleted && (i > TDEXIT)) {
			WAIT_ISR();
			do {
				status = bm_isr_status_read(portal);
			} while (status != BM_PIRQ_BSCN);
			bm_isr_status_clear(portal, BM_PIRQ_BSCN);
			status = bm_isr_status_read(portal);
			BUG_ON(status);
			bm_isr_uninhibit(portal);
			depleted = 0;
		}
		pr_info("Releasing %d bufs (%d-%d)\n", j, i - j, i - 1);
	}
	rcr_update(); /* RCR should be empty now */

	/****************/
	/* Acquire bufs */
	/****************/
	while (i) {
		int j;
		spin_lock_irq(&mc_lock);
		mc_start();
		mcc->acquire.bpid = bpid;
		mc_commit(BM_MCC_VERB_CMD_ACQUIRE |
			((i < TRELEASEMAX) ? i : TRELEASEMAX));
		j = mcr->verb & BM_MCR_VERB_ACQUIRE_BUFCOUNT;
		pr_info("Acquired %d bufs (%d remain)\n", j, i - j);
		BUG_ON(!j);
		BUG_ON(j > i);
		while (j--) {
			unsigned int idx;
			BUG_ON(mcr->acquire.bufs[j].hi != TADDRHI);
			idx = mcr->acquire.bufs[j].lo - TADDRLO;
			BUG_ON(idx >= TBUFS);
			BUG_ON(!BUFS_get(idx));
			BUFS_unset(idx);
			i--;
		}
		spin_unlock_irq(&mc_lock);
		if (!depleted && (i < TDENTRY)) {
			WAIT_ISR();
			do {
				status = bm_isr_status_read(portal);
			} while (status != BM_PIRQ_BSCN);
			bm_isr_status_clear(portal, BM_PIRQ_BSCN);
			status = bm_isr_status_read(portal);
			BUG_ON(status);
			bm_isr_uninhibit(portal);
			depleted = 1;
		}
	}

	/****************************/
	/* Acquire should give zero */
	/****************************/
	mc_start();
	mcc->acquire.bpid = bpid;
	mc_commit(BM_MCC_VERB_CMD_ACQUIRE | 1);
	pr_info("Acquiring 1 buf when pool is empty\n");
	BUG_ON((mcr->verb & BM_MCR_VERB_ACQUIRE_BUFCOUNT));
	pr_info("Acquire gave %d buffers when pool is empty\n",
		mcr->verb & BM_MCR_VERB_ACQUIRE_BUFCOUNT);

	if (big_loop--)
		goto again;

	bm_isr_bscn_mask(portal, bpid, 0);
	free_irq(config->irq, NULL);
	bm_rcr_finish(portal);
	bm_mc_finish(portal);
	bm_isr_status_clear(portal, -1);
	bm_isr_enable_write(portal, 0);
	bm_isr_finish(portal);

	bm_pool_free(bpid);
}

