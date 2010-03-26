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

#include "pme2_test.h"

MODULE_AUTHOR("Geoff Thorpe");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 (p4080) low-level self-test");

#define TTOKEN		0xc3

/* DQRR maxfill, and ring/data stashing booleans */
#define DQRR_MAXFILL	15
#define DQRR_STASH_RING	0
#define DQRR_STASH_DATA	0

static struct qm_portal *portal;
static struct qm_eqcr_entry *eq;
static struct qm_dqrr_entry *dq;
static struct qm_mc_command *mcc;
static struct qm_mc_result *mcr;

/* Helper for qm_mc_start() that checks the return code */
static void mc_start(void)
{
	mcc = qm_mc_start(portal);
	BUG_ON(!mcc);
}

/* Helper for qm_mc_result() that checks the response */
static void mc_commit(u8 verb)
{
	qm_mc_commit(portal, verb);
	do {
		mcr = qm_mc_result(portal);
	} while (!mcr);
	pr_info("command:\n");
	hexdump(mcc, sizeof(*mcc));
	pr_info("response:\n");
	hexdump(mcr, sizeof(*mcr));
	BUG_ON((mcr->verb & QM_MCR_VERB_MASK) != verb);
	BUG_ON(mcr->result != QM_MCR_RESULT_OK);
}

static struct qm_fd backup;

static void eqcr_update(void)
{
	qm_eqcr_cci_update(portal);
}

static void eqcr_start(void)
{
	#define SEED_INC 0xb9
	static u8 seeder = 0xff;
	eqcr_update();
	do {
		eq = qm_eqcr_start(portal);
	} while (!eq);
	memset(&eq->fd, seeder, sizeof(eq->fd));
	/* Qman apparently pays attention to format/length because of BYTE_CNT,
	 * so do the minimum to keep it happy (in particular, crop the length so
	 * that we don't hit overflow). */
	eq->fd.format = qm_fd_contig_big;
	eq->fd.length29 &= 0xffff;
	do {
		seeder += SEED_INC;
	} while (!seeder);
}

static void eqcr_commit(void)
{
	backup = eq->fd;
	hexdump(eq, sizeof(*eq));
	qm_eqcr_pci_commit(portal, QM_EQCR_VERB_CMD_ENQUEUE);
}

/* Macro'd so the BUG_ON() gives us the caller location */
#define dqrr_poll(check) \
do { \
	do { \
		qm_dqrr_pvb_update(portal); \
		dq = qm_dqrr_current(portal); \
	} while (!dq); \
	hexdump(dq, sizeof(*dq)); \
	if (check) { \
		backup.pid = dq->fd.pid; \
		backup.status = dq->fd.status; \
		BUG_ON(memcmp(&backup, &dq->fd, sizeof(backup))); \
	} \
} while(0)

static void dqrr_consume_and_next(void)
{
	qm_dqrr_next(portal);
	qm_dqrr_cci_consume_to_current(portal);
	dq = qm_dqrr_current(portal);
}

void pme2_test_low(struct qm_portal *__portal)
{
	const struct qm_portal_config *config;
	struct pme_hw_flow *hwflow = pme_hw_flow_new();
	struct pme_flow *test_flow = pme_sw_flow_new();
	struct pme_flow *test_flow2 = pme_sw_flow_new();
	u32 fqid_rx = qm_fq_new();
	u32 fqid_tx = qm_fq_new();

	pr_info("PME2: low-level test starting\n");
	BUG_ON(!hwflow || !test_flow || !test_flow2 || !fqid_rx || !fqid_tx);
	portal = __portal;
	config = qm_portal_config(portal);
	if (qm_eqcr_init(portal, qm_eqcr_pci, qm_eqcr_cci) ||
		qm_dqrr_init(portal,
			qm_dqrr_dpush, qm_dqrr_pvb, qm_dqrr_cci,
			DQRR_MAXFILL, DQRR_STASH_RING, DQRR_STASH_DATA) ||
		qm_mr_init(portal, qm_mr_pvb, qm_mr_cci) ||
		qm_mc_init(portal) || qm_isr_init(portal))
		panic("Portal setup failed");
	qm_dqrr_sdqcr_set(portal, QM_SDQCR_SOURCE_CHANNELS |
		QM_SDQCR_TYPE_PRIO_QOS | QM_SDQCR_TOKEN_SET(TTOKEN) |
		QM_SDQCR_CHANNELS_DEDICATED);
	/* Initialise output FQ and schedule it */
	mc_start();
	mcc->initfq.we_mask = QM_INITFQ_WE_DESTWQ | QM_INITFQ_WE_CONTEXTB;
	mcc->initfq.fqid = fqid_tx;
	mcc->initfq.count = 0;
	mcc->initfq.fqd.dest.channel = config->channel;
	mcc->initfq.fqd.dest.wq = 3;
	mcc->initfq.fqd.context_b = 0xdeadbeef;
	pr_info("Init output FQ\n");
	mc_commit(QM_MCC_VERB_INITFQ_SCHED);
	/* Initialise input FQ for Direct Action mode and schedule it */
	mc_start();
	pme_initfq(&mcc->initfq, NULL, 3, 0, fqid_tx);
	mcc->initfq.fqid = fqid_rx;
	mcc->initfq.count = 0;
	pr_info("Init input FQ\n");
	mc_commit(QM_MCC_VERB_INITFQ_SCHED);
	/* Send a NOP */
	eqcr_start();
	eq->fqid = fqid_rx;
	pme_fd_cmd_nop(&eq->fd);
	pr_info("Enqueuing NOP\n");
	eqcr_commit();
	/* Poll for the NOP */
	pr_info("Dequeuing NOP\n");
	dqrr_poll(0);
	BUG_ON((dq->verb & QM_DQRR_VERB_MASK) != QM_DQRR_VERB_FRAME_DEQUEUE);
	BUG_ON(dq->stat != (QM_DQRR_STAT_FQ_EMPTY | QM_DQRR_STAT_FD_VALID));
	BUG_ON(dq->tok != TTOKEN);
	BUG_ON(dq->fqid != fqid_tx);
	BUG_ON(dq->contextB != 0xdeadbeef);
	BUG_ON(pme_fd_res_status(&dq->fd) != pme_status_ok);
	BUG_ON(pme_fd_res_flags(&dq->fd) & (PME_STATUS_UNRELIABLE |
				PME_STATUS_TRUNCATED));
	dqrr_consume_and_next();
	/* Retire and OOS the input FQ */
	mc_start();
	mcc->alterfq.fqid = fqid_rx;
	pr_info("Retire input FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_RETIRE);
	BUG_ON(mcr->alterfq.fqs & (QM_MCR_FQS_ORLPRESENT |
				QM_MCR_FQS_NOTEMPTY));
	mc_start();
	mcc->alterfq.fqid = fqid_rx;
	pr_info("OOS input FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_OOS);
	/* Initialise input FQ for Flow mode and schedule it */
	mc_start();
	pme_initfq(&mcc->initfq, hwflow, 3, 0, fqid_tx);
	mcc->initfq.fqid = fqid_rx;
	mcc->initfq.count = 0;
	pr_info("Init input FQ\n");
	mc_commit(QM_MCC_VERB_INITFQ_SCHED);
	/* Send a FCW to initialise the flow */
	memset(test_flow, 0, sizeof(*test_flow));
	test_flow->srvm = 2;
	test_flow->seqnum_hi = 0xabba;
	test_flow->seqnum_lo = 0xdeadbeef;
	test_flow->clim = 0xabcd;
	test_flow->mlim = 0x9876;
	eqcr_start();
	eq->fqid = fqid_rx;
	pme_fd_cmd_fcw(&eq->fd, PME_CMD_FCW_ALL, test_flow, NULL);
	pr_info("Enqueuing FCW\n");
	eqcr_commit();
	/* Poll for the FCW response */
	pr_info("Dequeuing FCW\n");
	dqrr_poll(1);
/*	BUG_ON((dq->stat & QM_DQRR_VERB_MASK) != (QM_DQRR_STAT_FQ_EMPTY |
						QM_DQRR_STAT_FD_VALID)); */
	BUG_ON(dq->tok != TTOKEN);
	BUG_ON(dq->fqid != fqid_tx);
	BUG_ON(dq->contextB != 0xdeadbeef);
	BUG_ON(pme_fd_res_status(&dq->fd) != pme_status_ok);
	BUG_ON(pme_fd_res_flags(&dq->fd) & (PME_STATUS_UNRELIABLE |
				PME_STATUS_TRUNCATED));
	dqrr_consume_and_next();
	/* Send a FCR to read the flow back */
	memset(test_flow2, 0xff, sizeof(*test_flow2));
	eqcr_start();
	eq->fqid = fqid_rx;
	pme_fd_cmd_fcr(&eq->fd, test_flow2);
	pr_info("Enqueuing FCR\n");
	eqcr_commit();
	/* Poll for the FCR response */
	pr_info("Dequeuing FCR\n");
	dqrr_poll(1);
/*	BUG_ON((dq->stat & QM_DQRR_VERB_MASK) != (QM_DQRR_STAT_FQ_EMPTY |
						QM_DQRR_STAT_FD_VALID)); */
	BUG_ON(dq->tok != TTOKEN);
	BUG_ON(dq->fqid != fqid_tx);
	BUG_ON(dq->contextB != 0xdeadbeef);
	BUG_ON(pme_fd_res_status(&dq->fd) != pme_status_ok);
	BUG_ON(pme_fd_res_flags(&dq->fd) & (PME_STATUS_UNRELIABLE |
				PME_STATUS_TRUNCATED));
	dqrr_consume_and_next();
	/* Send a NOP through the flow */
	eqcr_start();
	eq->fqid = fqid_rx;
	pme_fd_cmd_nop(&eq->fd);
	pr_info("Enqueuing NOP\n");
	eqcr_commit();
	/* Poll for the NOP response */
	pr_info("Dequeuing NOP\n");
	dqrr_poll(0);
/*	BUG_ON((dq->stat & QM_DQRR_VERB_MASK) != (QM_DQRR_STAT_FQ_EMPTY |
						QM_DQRR_STAT_FD_VALID)); */
	BUG_ON(dq->tok != TTOKEN);
	BUG_ON(dq->fqid != fqid_tx);
	BUG_ON(dq->contextB != 0xdeadbeef);
	BUG_ON(pme_fd_res_status(&dq->fd) != pme_status_ok);
	BUG_ON(pme_fd_res_flags(&dq->fd) & (PME_STATUS_UNRELIABLE |
				PME_STATUS_TRUNCATED));
	dqrr_consume_and_next();
	/* Retire and OOS the input FQ */
	mc_start();
	mcc->alterfq.fqid = fqid_rx;
	pr_info("Retire input FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_RETIRE);
	BUG_ON(mcr->alterfq.fqs & (QM_MCR_FQS_ORLPRESENT |
				QM_MCR_FQS_NOTEMPTY));
	mc_start();
	mcc->alterfq.fqid = fqid_rx;
	pr_info("OOS input FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_OOS);
	/* Retire and OOS the output FQ */
	mc_start();
	mcc->alterfq.fqid = fqid_tx;
	pr_info("Retire output FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_RETIRE);
	BUG_ON(mcr->alterfq.fqs & (QM_MCR_FQS_ORLPRESENT |
				QM_MCR_FQS_NOTEMPTY));
	mc_start();
	mcc->alterfq.fqid = fqid_tx;
	pr_info("OOS output FQ\n");
	mc_commit(QM_MCC_VERB_ALTER_OOS);

	eqcr_update();
	qm_eqcr_finish(portal);
	qm_dqrr_finish(portal);
	qm_mr_finish(portal);
	qm_mc_finish(portal);
	qm_isr_finish(portal);
	pr_info("PME2: low-level test done\n");
}

static int pme2_test_low_init(void)
{
	int big_loop = 2;
	u8 num = qm_portal_num();
	const struct qm_portal_config *config = NULL;
	struct qm_portal *portal;

	while (!config && (num-- > 0)) {
		portal = qm_portal_get(num);
		config = qm_portal_config(portal);
		if (!config->bound)
			pr_info("Portal %d is available, using it\n", num);
		else
			config = NULL;
	}
	if (!config) {
		pr_err("No Qman portals available!\n");
		return -ENOSYS;
	}
	while (big_loop--)
		pme2_test_low(portal);
	return 0;
}

static void pme2_test_low_exit(void)
{
}

module_init(pme2_test_low_init);
module_exit(pme2_test_low_exit);
