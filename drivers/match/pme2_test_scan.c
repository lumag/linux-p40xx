/* Copyright (c) 2009, Freescale Semiconductor, Inc.
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

static u8 scan_data[] = {
	0x41,0x42,0x43,0x44,0x45
};

static u8 result_data[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00
};

static u8 scan_result_direct_mode_inc_mode[] = {
	0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
	0x00,0x00
};

static u8 fl_ctx_exp[] = {
	0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
	0x00,0x00,0x00,0x00
};

struct scan_ctx {
	struct pme_ctx base_ctx;
	struct qm_fd result_fd;
};

static DECLARE_COMPLETION(scan_comp);

static void scan_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_token *ctx_token)
{
	struct scan_ctx *my_ctx = (struct scan_ctx *)ctx;
	pr_info("st: scan_cb() invoked, fd;!\n");
	hexdump(fd, sizeof(*fd));
	memcpy(&my_ctx->result_fd, fd, sizeof(*fd));
	complete(&scan_comp);
}

#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID

static struct bman_pool *pool;
static u32 pme_bpid;
static void *bman_buffers_virt_base;
static dma_addr_t bman_buffers_phys_base;

static void release_buffer(dma_addr_t addr)
{
	struct bm_buffer bufs_in;

	memset(&bufs_in, 0, sizeof(struct bm_buffer));
	bufs_in.lo = addr;
	if (bman_release(pool, &bufs_in, 1, BMAN_RELEASE_FLAG_WAIT))
		panic("bman_release() failed\n");
}

static void empty_buffer(void)
{
	struct bm_buffer bufs_in;
	int ret;

	do {
		ret = bman_acquire(pool, &bufs_in, 1, 0);
		if (!ret)
			pr_info("st: Acquired buffer\n");
	} while (!ret);
}
#endif /*CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID*/

void pme2_test_scan(void)
{
	struct pme_flow *flow;
	struct scan_ctx a_scan_ctx = {
		.base_ctx = {
			.cb = scan_cb
		}
	};
	struct qm_fd fd;
	struct qm_sg_entry sg_table[2];
	int ret;
	enum pme_status status;
	struct pme_ctx_token token;
	u8 *scan_result;
	u32 scan_result_size;

	scan_result = scan_result_direct_mode_inc_mode;
	scan_result_size = sizeof(scan_result_direct_mode_inc_mode);

	/**********************************************************************/
	/**********************************************************************/
	/********************* Direct Mode ************************************/
	/**********************************************************************/
	/**********************************************************************/
	ret = pme_ctx_init(&a_scan_ctx.base_ctx,
			PME_CTX_FLAG_DIRECT | PME_CTX_FLAG_LOCAL,
			0, 4, 4, 0, NULL);
	BUG_ON(ret);
	/* enable the context */
	pme_ctx_enable(&a_scan_ctx.base_ctx);

	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	/* build the result */
	sg_table[0].addr_lo = pme_map(result_data);
	sg_table[0].length = sizeof(result_data);
	sg_table[1].addr_lo = pme_map(scan_data);
	sg_table[1].length = sizeof(scan_data);
	sg_table[1].final = 1;

	fd._format2 = qm_fd_compound;
	fd.addr_lo = pme_map(sg_table);

	pr_info("st: Send scan request\n");
	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	pr_info("st: Response scan %d\n", ret);
	wait_for_completion(&scan_comp);

	pr_info("st: Status is fd.status %x\n",
			a_scan_ctx.result_fd.status);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_info("st: Scan status failed %d\n", status);
		BUG_ON(1);
	}
	if (memcmp(scan_result,result_data, scan_result_size) != 0) {
		pr_info("st: Scan result not expected, Expected\n");
		hexdump(scan_result, scan_result_size);
		pr_info("st: Received...\n");
		hexdump(result_data, sizeof(result_data));
		BUG_ON(1);
	}
	/* Test truncation test */

	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	fd.length20 = sizeof(scan_data);
	fd.addr_lo = pme_map(scan_data);

	pr_info("st: Send scan request\n");

	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);

	pr_info("st: Response scan %d\n", ret);
	wait_for_completion(&scan_comp);

	pr_info("st: Status is fd.status %x\n",
			a_scan_ctx.result_fd.status);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	pr_info("st: Scan status %x\n", status);

	/* Check the response...expect truncation bit to be set */
	if (!(pme_fd_res_flags(&a_scan_ctx.result_fd) & PME_STATUS_TRUNCATED)) {
		pr_info("st: Scan result failed...expected trunc\n");
		BUG_ON(1);
	}
	pr_info("st: Simple scan test Passed\n");

	/* Disable */
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT);
	BUG_ON(ret);

	/* Check with bman */
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	/* reconfigure */
	ret = pme_ctx_reconfigure_tx(&a_scan_ctx.base_ctx, pme_bpid, 5);
	BUG_ON(ret);
	ret = pme_ctx_enable(&a_scan_ctx.base_ctx);
	BUG_ON(ret);
	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	/* build the result */
	/* result is all zero...use bman */
	sg_table[1].addr_lo = pme_map(scan_data);
	sg_table[1].length = sizeof(scan_data);
	sg_table[1].final = 1;

	fd._format2 = qm_fd_compound;
	fd.addr_lo = pme_map(sg_table);

	pr_info("st: Send scan with bpid response request\n");
	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	pr_info("st: Response scan %d\n", ret);
	wait_for_completion(&scan_comp);

	pr_info("st: Status is fd.status %x\n",
			a_scan_ctx.result_fd.status);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_info("st: Scan status failed %d\n", status);
		BUG_ON(1);
	}

	/* sg result should point to bman buffer */
	pr_info("st: sg result should point to bman buffer 0x%x\n",
			sg_table[0].addr_lo);
	if (!sg_table[0].addr_lo)
		BUG_ON(1);

	if (memcmp(scan_result, bman_buffers_virt_base, scan_result_size)
			!= 0) {
		pr_info("st: Scan result not expected, Expected\n");
		hexdump(scan_result, scan_result_size);
		pr_info("st: Received...\n");
		hexdump(bman_buffers_virt_base, scan_result_size);
		BUG_ON(1);
	}

	release_buffer(sg_table[0].addr_lo);
	pr_info("st: Released to bman\n");

	/* Disable */
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT);
	BUG_ON(ret);
#endif
	pme_ctx_finish(&a_scan_ctx.base_ctx);

	/**********************************************************************/
	/**********************************************************************/
	/*********************** Flow Mode ************************************/
	/**********************************************************************/
	/**********************************************************************/
	pr_info("st: Start Flow Mode Test\n");

	flow = pme_sw_flow_new();
	BUG_ON(!flow);
	ret = pme_ctx_init(&a_scan_ctx.base_ctx,
		PME_CTX_FLAG_LOCAL, 0, 4, 4, 0, NULL);
	BUG_ON(ret);

	/* enable the context */
	pme_ctx_enable(&a_scan_ctx.base_ctx);
	pr_info("st: Context Enabled\n");

	/* read back flow settings */
	{
		struct pme_flow* rb_flow;
		rb_flow = pme_sw_flow_new();
		memset(rb_flow, 0, sizeof(struct pme_flow));
		pr_info("st: Initial rb_flow\n");
		hexdump(rb_flow, sizeof(*rb_flow));

		ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT |
			PME_CTX_OP_WAIT_INT |
			PME_CMD_FCW_ALL, rb_flow);
		BUG_ON(ret);
		if (memcmp(rb_flow,fl_ctx_exp, sizeof(*rb_flow)) != 0) {
			pr_info("st: Flow Context Read FAIL\n");
			pr_info("st: Expected\n");
			hexdump(fl_ctx_exp, sizeof(fl_ctx_exp));
			pr_info("st: Received...\n");
			hexdump(rb_flow, sizeof(*rb_flow));
			BUG_ON(1);
		} else {
			pr_info("st: Flow Context Read OK\n");
		}
		pme_sw_flow_free(rb_flow);
	}


	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	/* build the result */
	sg_table[0].addr_lo = pme_map(result_data);
	sg_table[0].length = sizeof(result_data);
	sg_table[1].addr_lo = pme_map(scan_data);
	sg_table[1].length = sizeof(scan_data);
	sg_table[1].final = 1;

	fd._format2 = qm_fd_compound;
	fd.addr_lo = pme_map(sg_table);

	pr_info("st: Send scan request\n");
	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);

	pr_info("st: Response scan %d\n", ret);
	wait_for_completion(&scan_comp);

	pr_info("st: Status is fd.status %x\n",
			a_scan_ctx.result_fd.status);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_info("st: Scan status failed %d\n", status);
		BUG_ON(1);
	}

	if (memcmp(scan_result,result_data, scan_result_size) != 0) {
		pr_info("st: Scan result not expected, Expected\n");
		hexdump(scan_result, scan_result_size);
		pr_info("st: Received...\n");
		hexdump(result_data, sizeof(result_data));
		BUG_ON(1);
	}

	/* read back flow settings */
	{
		struct pme_flow *rb_flow;
		rb_flow = pme_sw_flow_new();
		memset(rb_flow, 0, sizeof(struct pme_flow));
		pr_info("st: Initial rb_flow\n");
		hexdump(rb_flow, sizeof(*rb_flow));

		ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT |
			PME_CTX_OP_WAIT_INT |
			PME_CMD_FCW_ALL, rb_flow);
		BUG_ON(ret);
		if (memcmp(rb_flow,fl_ctx_exp, sizeof(*rb_flow)) != 0) {
			pr_info("st: Flow Context Read FAIL\n");
			pr_info("st: Expected\n");
			hexdump(fl_ctx_exp, sizeof(fl_ctx_exp));
			pr_info("st: Received\n");
			hexdump(rb_flow, sizeof(*rb_flow));
			BUG_ON(1);
		} else {
			pr_info("st: Flow Context Read OK\n");
		}
		pme_sw_flow_free(rb_flow);
	}
	/* Test truncation test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));

	fd.length20 = sizeof(scan_data);
	fd.addr_lo = pme_map(scan_data);

	pr_info("st: Send scan request\n");
	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);

	pr_info("st: Response scan %d\n", ret);
	wait_for_completion(&scan_comp);

	pr_info("st: Status is fd.status %x\n",
			a_scan_ctx.result_fd.status);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	 pr_info("Scan status %x\n",status);

	/* Check the response...expect truncation bit to be set */
	if (!(pme_fd_res_flags(&a_scan_ctx.result_fd) & PME_STATUS_TRUNCATED)) {
		pr_info("st: Scan result failed...expected trunc\n");
		BUG_ON(1);
	}
	pr_info("st: Simple scan test Passed\n");

	/* read back flow settings */
	{
		struct pme_flow *rb_flow;
		rb_flow = kmalloc(sizeof(struct pme_flow), GFP_KERNEL);
		memset(rb_flow, 0, sizeof(struct pme_flow));
		ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT |
			PME_CTX_OP_WAIT_INT |
			PME_CMD_FCW_ALL, rb_flow);
		BUG_ON(ret);
		pr_info("st: read Flow Context;\n");
		hexdump(rb_flow, sizeof(*rb_flow));
		kfree(rb_flow);
	}

	/* Disable */
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT | PME_CTX_OP_WAIT_INT);
	BUG_ON(ret);
	pme_ctx_finish(&a_scan_ctx.base_ctx);
	pme_sw_flow_free(flow);

	pr_info("st: Scan Test Passed\n");
}

static int pme2_test_scan_init(void)
{
	int big_loop = 2;
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	u32 bpid_size = CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID_SIZE;

	struct bman_pool_params pparams = {
		.flags = BMAN_POOL_FLAG_DYNAMIC_BPID,
		.thresholds = {
			0,
			0,
			0,
			0
		}
	};
	if (!pme2_have_control()) {
		pr_err("st: Not the ctrl-plane\n");
		return 0;
	}

	pr_info("st: About to allocate bpool\n");
	pool = bman_new_pool(&pparams);
	BUG_ON(!pool);
	pme_bpid = bman_get_params(pool)->bpid;
	pr_info("st: Allocate buffer pool id %d\n", pme_bpid);
	pr_info("st: Allocate buffer of size %d\n", 1<<(bpid_size+5));
	bman_buffers_virt_base = kmalloc(1<<(bpid_size+5), GFP_KERNEL);
	bman_buffers_phys_base = pme_map(bman_buffers_virt_base);
	pr_info("st: virt address %p\n", bman_buffers_virt_base);
	pr_info("st: physical address 0x%x\n", bman_buffers_phys_base);
	pr_info("st: Allocate buffer of size 0x%x\n", 1<<(bpid_size+5));

	release_buffer(bman_buffers_phys_base);
	pr_info("st: Released to bman\n");

	/* Configure the buffer pool */
	pr_info("st: Config bpid %d with size %u\n", pme_bpid, bpid_size);
	pme_attr_set(pme_attr_bsc(pme_bpid), bpid_size);
	/* realease to the specified buffer pool */
#endif
	if (!pme2_have_control()) {
		pr_err("st: Not the ctrl-plane\n");
		return 0;
	}

	while (big_loop--)
		pme2_test_scan();
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	pme_attr_set(pme_attr_bsc(pme_bpid), 0);
	empty_buffer();
	bman_free_pool(pool);
	kfree(bman_buffers_virt_base);
#endif
	return 0;
}

static void pme2_test_scan_exit(void)
{
}

module_init(pme2_test_scan_init);
module_exit(pme2_test_scan_exit);
