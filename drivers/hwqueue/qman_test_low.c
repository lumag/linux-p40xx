/* Copyright (c) 2008-2010 Freescale Semiconductor, Inc.
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

#include "qman_test.h"

/* Bug or missing-feature workarounds */
#define BUG_NO_EQCR_ITR
#define BUG_NO_REASSERT_DQAVAIL

/* DQRR maxfill, and ring/data stashing booleans */
#define DQRR_MAXFILL	15

/* Test constants */
#define TWQ		3	/* 0..7, ie. within the channel */
#define TCONTEXTB	0xabcd9876
#define TFRAMES		3	/* Number enqueued */
#define TEQVERB		QM_EQCR_VERB_CMD_ENQUEUE
#define TTOKEN		0xab

/* Global variables make life simpler */
static struct qm_portal *portal;
static struct qm_eqcr_entry *eq;
static struct qm_dqrr_entry *dq;
static struct qm_mr_entry *mr;
static struct qm_mc_command *mcc;
static struct qm_mc_result *mcr;
static DECLARE_WAIT_QUEUE_HEAD(queue);
static int isr_count;

#ifdef CONFIG_FSL_QMAN_FQALLOCATOR

/* Use Bman for dynamic FQ allocation (BPID==0) */
#include <linux/fsl_bman.h>
static struct bman_pool *fq_pool;
static const struct bman_pool_params fq_pool_params;
static u32 fqid;
static void alloc_fqid(void)
{
	struct bm_buffer buf;
	int ret;
	fq_pool = bman_new_pool(&fq_pool_params);
	BUG_ON(!fq_pool);
	ret = bman_acquire(fq_pool, &buf, 1, 0);
	BUG_ON(ret != 1);
	fqid = buf.lo;
}
static void free_fqid(void)
{
	struct bm_buffer buf = {
		.hi = 0,
		.lo = fqid
	};
	int ret = bman_release(fq_pool, &buf, 1, BMAN_RELEASE_FLAG_WAIT);
	BUG_ON(ret);
	bman_free_pool(fq_pool);
}

#else /* !defined(CONFIG_FSL_QMAN_FQALLOCATOR) */

/* No dynamic allocation, use FQID==1 */
static u32 fqid = 1;
#define alloc_fqid()	0
#define free_fqid()	do { ; } while(0)

#endif /* !defined(CONFIG_FSL_QMAN_FQALLOCATOR) */

/* Boolean switch for handling EQCR_ITR */
#ifndef BUG_NO_EQCR_ITR
static int eqcr_thresh_on;
#endif

/* Test frame-descriptor, and another one to track dequeues */
static struct qm_fd fd, fd_dq;

/* Helpers for initialising and "incrementing" a frame descriptor */
static void fd_init(struct qm_fd *__fd)
{
	__fd->addr_hi = 0xab;		/* high 16-bits */
	__fd->addr_lo = 0xdeadbeef;	/* low 32-bits */
	__fd->format = qm_fd_contig_big;
	__fd->length29 = 0x0000ffff;
	__fd->cmd = 0xfeedf00d;
}

static void fd_inc(struct qm_fd *__fd)
{
	__fd->addr_lo++;
	__fd->addr_hi--;
	__fd->length29--;
	__fd->cmd++;
}

/* Helper for qm_mc_start() that checks the return code */
static void mc_start(void)
{
	mcc = qm_mc_start(portal);
	BUG_ON(!mcc);
}

/* Helper for qm_mc_result() that checks the response */
#define mc_commit(v) \
do { \
	qm_mc_commit(portal, v); \
	do { \
		mcr = qm_mc_result(portal); \
	} while (!mcr); \
	BUG_ON((mcr->verb & QM_MCR_VERB_MASK) != v); \
	BUG_ON(mcr->result != QM_MCR_RESULT_OK); \
} while(0)

/* Track EQCR consumption */
static void eqcr_update(void)
{
#ifndef BUG_NO_EQCR_ITR
	u32 status = qm_isr_status_read(portal);
	if (status & QM_PIRQ_EQRI) {
		int progress = qm_eqcr_cci_update(portal);
		BUG_ON(!progress);
		BUG_ON(!eqcr_thresh_on);
		qm_eqcr_set_ithresh(portal, 0);
		eqcr_thresh_on = 0;
		pr_info("Auto-update of EQCR consumption\n");
		qm_isr_status_clear(portal, progress & QM_PIRQ_EQRI);
	}
#else
	qm_eqcr_cci_update(portal);
#endif
}

/* Helper for qm_eqcr_start() that tracks ring consumption and checks the
 * return code */
static void eqcr_start(void)
{
	/* If there are consumed EQCR entries, track them now. The alternative
	 * is to catch an error in qm_eqcr_start(), track consume entries then,
	 * and then retry qm_eqcr_start(). */
	do {
		eqcr_update();
		eq = qm_eqcr_start(portal);
	} while (!eq);
}

/* Wait for EQCR to empty */
static void eqcr_empty(void)
{
	do {
		eqcr_update();
	} while (qm_eqcr_get_fill(portal));
}

/* Helper for qm_eqcr_pvb_commit() */
static void eqcr_commit(void)
{
	qm_eqcr_pvb_commit(portal, TEQVERB);
#ifndef BUG_NO_EQCR_ITR
	if (!eqcr_thresh_on && (qm_eqcr_get_avail(portal) < 2)) {
		eqcr_thresh_on = 1;
		qm_eqcr_set_ithresh(portal, 1);
	}
#endif
}

/* Track DQRR consumption */
static void dqrr_update(void)
{
	do {
		qm_dqrr_pvb_update(portal);
		dq = qm_dqrr_current(portal);
		if (!dq)
			qm_dqrr_pvb_prefetch(portal);
	} while (!dq);
}

/* Helper for qm_dqrr_cci_consume() */
static void dqrr_consume_and_next(void)
{
	qm_dqrr_next(portal);
	qm_dqrr_cci_consume_to_current(portal);
	qm_dqrr_pvb_prefetch(portal);
}

static void mr_update(void)
{
	do {
		qm_mr_pvb_update(portal);
		mr = qm_mr_current(portal);
	} while (!mr);
}

static void mr_consume_and_next(void)
{
	qm_mr_next(portal);
	qm_mr_cci_consume_to_current(portal);
}

#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
static irqreturn_t portal_isr(int irq, void *ptr)
{
	pr_info("QMAN portal interrupt, isr_count=%d->%d\n", isr_count,
		isr_count + 1);
	isr_count++;
	qm_isr_inhibit(portal);
	wake_up(&queue);
	return IRQ_HANDLED;
}
#endif

void qman_test_low(struct qm_portal *__p)
{
	cpumask_t oldmask = current->cpus_allowed, newmask = CPU_MASK_NONE;
	const struct qm_portal_config *config = qm_portal_config(__p);
	int i;
#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	u32 status;
#endif

	portal = __p;
	fd_init(&fd);
	fd_init(&fd_dq);
	isr_count = 0;

	pr_info("qman_test_low starting\n");

	/* If the portal is affine, run on the corresponding cpu */
	if (config->cpu != -1) {
		cpu_set(config->cpu, newmask);
		if (set_cpus_allowed(current, newmask))
			panic("can't schedule to affine cpu");
	}

	alloc_fqid();
	/*********************/
	/* Initialise portal */
	/*********************/
	if (qm_eqcr_init(portal, qm_eqcr_pvb, qm_eqcr_cci) ||
#ifdef CONFIG_FSL_QMAN_TEST_LOW_BUG_STASHING
		qm_dqrr_init(portal,
			qm_dqrr_dpush, qm_dqrr_pvb, qm_dqrr_cci,
			DQRR_MAXFILL, 0, 0) ||
#else
		qm_dqrr_init(portal,
			qm_dqrr_dpush, qm_dqrr_pvb, qm_dqrr_cci,
			DQRR_MAXFILL,
			(config->cpu == -1) ? 0 : 1,
			(config->cpu == -1) ? 0 : 1) ||
#endif
		qm_mr_init(portal, qm_mr_pvb, qm_mr_cci) ||
		qm_mc_init(portal) || qm_isr_init(portal))
		panic("Portal setup failed");
	/* Set interrupt register values (eg. for post reboot) */
	qm_isr_enable_write(portal, 0);
	qm_isr_disable_write(portal, 0);
	qm_isr_uninhibit(portal);
	qm_isr_status_clear(portal, 0xffffffff);

	pr_info("low-level test, start ccmode\n");

#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	if (request_irq(config->irq, portal_isr, 0, "Qman portal 0", NULL))
		panic("Can't register Qman portal 0 IRQ");
#endif
	pr_info("Portal %d channel i/faces initialised\n", config->channel);

	/*****************/
	/* Initialise FQ */
	/*****************/
	mc_start();
	mcc->initfq.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_CONTEXTB;
	mcc->initfq.fqid = fqid;
	mcc->initfq.count = 0;
	/* mcc->initfq.fqd.fq_ctrl = 0; */ /* (QM_FQCTRL_***) */
	mcc->initfq.fqd.dest.channel = config->channel;
	mcc->initfq.fqd.dest.wq = TWQ;
	mcc->initfq.fqd.context_b = TCONTEXTB;
	mc_commit(QM_MCC_VERB_INITFQ_PARKED);
	pr_info("FQ %d initialised for channel %d, wq %d\n", fqid,
		config->channel, TWQ);

#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* The portal's (interrupt) status register should be zero */
	status = qm_isr_status_read(portal);
	BUG_ON(status);
#endif

	/**************************/
	/* Enqueue TFRAMES frames */
	/**************************/
	for (i = 0; i < TFRAMES; i++) {
		/* Enqueue the test frame-descriptor */
		eqcr_start();
		eq->fqid = fqid;
		memcpy(&eq->fd, &fd, sizeof(fd));
		eqcr_commit();
		pr_info("Enqueued frame %d: %08x\n", i, fd.addr_lo);
		/* Modify the frame-descriptor for next time */
		fd_inc(&fd);
	}
	eqcr_empty();

#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* The portal's (interrupt) status register should be zero */
	status = qm_isr_status_read(portal);
	BUG_ON(status);
#endif

	/***************/
	/* Schedule FQ */
	/***************/
#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* enable the interrupt source before it asserts */
	qm_isr_enable_write(portal, QM_DQAVAIL_PORTAL);
	qm_isr_uninhibit(portal);
#endif
	mc_start();
	mcc->alterfq.fqid = fqid;
	mc_commit(QM_MCC_VERB_ALTER_SCHED);
	pr_info("FQ %d scheduled\n", fqid);
#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* The interrupt should have fired immediately. */
	wait_event(queue, isr_count == 1);
	/* The status register should show DQAVAIL. */
	do {
		status = qm_isr_status_read(portal);
	} while (status != QM_DQAVAIL_PORTAL);
	/* Writing to clear should fail due to an immediate reassertion */
#ifndef BUG_NO_REASSERT_DQAVAIL
	qm_isr_status_clear(portal, status);
	do {
		status = qm_isr_status_read(portal);
	} while (status != QM_DQAVAIL_PORTAL);
#endif
	/* Zeroing the enable register before unihibiting should prevent any
	 * interrupt */
	qm_isr_enable_write(portal, 0);
	qm_isr_uninhibit(portal);
	BUG_ON(isr_count != 1);
#endif

	/******************************/
	/* SDQCR the remaining frames */
	/******************************/

	/* Initiate SDQCR (static dequeue command) */
	qm_dqrr_sdqcr_set(portal, QM_SDQCR_SOURCE_CHANNELS |
		QM_SDQCR_TYPE_PRIO_QOS | QM_SDQCR_TOKEN_SET(TTOKEN) |
		QM_SDQCR_CHANNELS_DEDICATED);

#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* The status register should have the sticky 'DQAVAIL' from before, and
	 * maybe DQRI (it's on its way). */
	status = qm_isr_status_read(portal);
	BUG_ON(!(status & QM_DQAVAIL_PORTAL));
	qm_isr_enable_write(portal, QM_PIRQ_DQRI);
	wait_event(queue, isr_count == 2);
	do {
		status = qm_isr_status_read(portal);
	} while (!(status & QM_PIRQ_DQRI));
	/* Writing to clear should fail due to an immediate reassertion. NB
	 * also, DQAVAIL_PORTAL mightn't disappear right away, the dequeues are
	 * delayed so the FQ may stay truly-scheduled for a bit... */
	qm_isr_status_clear(portal, status);
	do {
		status = qm_isr_status_read(portal);
	} while (!(status & QM_PIRQ_DQRI));
	/* Zeroing the enable register before unihibiting should prevent any
	 * interrupt */
	qm_isr_enable_write(portal, 0);
	qm_isr_uninhibit(portal);
	BUG_ON(isr_count != 2);
#endif
	for (i = 0; i < TFRAMES; i++)
	{
		dqrr_update();
		BUG_ON((dq->verb & QM_DQRR_VERB_MASK) !=
				QM_DQRR_VERB_FRAME_DEQUEUE);
		pr_info("Dequeued SDQCR frame %d: %08x\n", i, dq->fd.addr_lo);
		if ((i + 1) == TFRAMES)
			BUG_ON(!(dq->stat & QM_DQRR_STAT_FQ_EMPTY));
		else
			BUG_ON(dq->stat & QM_DQRR_STAT_FQ_EMPTY);
		BUG_ON(dq->stat & QM_DQRR_STAT_FQ_HELDACTIVE);
		BUG_ON(dq->stat & QM_DQRR_STAT_FQ_FORCEELIGIBLE);
		BUG_ON(!(dq->stat & QM_DQRR_STAT_FD_VALID));
		BUG_ON(dq->stat & QM_DQRR_STAT_UNSCHEDULED);
		BUG_ON(dq->tok != TTOKEN);
		BUG_ON(dq->fqid != fqid);
		BUG_ON(dq->contextB != TCONTEXTB);
		fd_dq.liodn_offset = dq->fd.liodn_offset;
		fd_dq.eliodn_offset = dq->fd.eliodn_offset;
		BUG_ON(memcmp(&dq->fd, &fd_dq, sizeof(fd_dq)));
		/* Modify the frame-descriptor for next time */
		fd_inc(&fd_dq);
		dqrr_consume_and_next();
	}
#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	/* Clear stick bits from the status register, and it should
	 * now remain zero. */
	status = qm_isr_status_read(portal);
	qm_isr_status_clear(portal, status);
	status = qm_isr_status_read(portal);
	BUG_ON(status);
#endif

	pr_info("low-level test, end ccmode\n");

	/*************/
	/* Retire FQ */
	/*************/
	mc_start();
	mcc->alterfq.fqid = fqid;
	mc_commit(QM_MCC_VERB_ALTER_RETIRE);
	pr_info("FQ %d retired\n", fqid);

	/*****************/
	/* Consume FQRNI */
	/*****************/
	mr_update();
	BUG_ON((mr->verb & QM_MR_VERB_TYPE_MASK) != QM_MR_VERB_FQRNI);
	pr_info("Received FQRNI message\n");
	hexdump(mr, 32);
	BUG_ON(mr->fq.fqid != fqid);
	BUG_ON(mr->fq.fqs);
	BUG_ON(mr->fq.contextB != TCONTEXTB);
	mr_consume_and_next();

	/**********/
	/* OOS FQ */
	/**********/
	mc_start();
	mcc->alterfq.fqid = fqid;
	mc_commit(QM_MCC_VERB_ALTER_OOS);
	pr_info("FQ %d OOS'd\n", fqid);

	/************/
	/* Teardown */
	/************/
	eqcr_empty();
#ifndef CONFIG_FSL_QMAN_TEST_LOW_BUG_INTERRUPTS
	free_irq(config->irq, NULL);
#endif
	qm_eqcr_finish(portal);
	qm_dqrr_finish(portal);
	qm_mr_finish(portal);
	qm_mc_finish(portal);
	qm_isr_finish(portal);
	free_fqid();
	if (config->cpu != -1)
		if (set_cpus_allowed(current, oldmask))
			panic("can't restore old cpu mask");
	pr_info("qman_test_low finished\n");
}

