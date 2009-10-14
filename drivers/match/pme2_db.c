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

#include "pme2_private.h"

/* Forward declaration */
static struct miscdevice fsl_pme2_db_dev;

/* Global spinlock for handling exclusive inc/dec */
static DEFINE_SPINLOCK(exclusive_lock);

/* Private structure that is allocated for each open that is done on the
 * pme_db device. This is used to maintain the state of a database session */
struct db_session {
	/* The ctx that is needed to communicate with the pme high level */
	struct pme_ctx ctx;
	/* Used to track the EXCLUSIVE_INC and EXCLUSIVE_DEC ioctls */
	unsigned int exclusive_counter;
};

struct cmd_token {
	/* pme high level token */
	struct pme_ctx_token hl_token;
	/* data */
	struct qm_fd rx_fd;
	/* Completion interface */
	struct completion cb_done;
};

/* PME Compound Frame Index */
#define INPUT_FRM	1
#define OUTPUT_FRM	0

/* Callback for database operations */
static void db_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
				struct pme_ctx_token *ctx_token)
{
	struct cmd_token *token = (struct cmd_token *)ctx_token;
	token->rx_fd = *fd;
	complete(&token->cb_done);
}

struct ctrl_op {
	struct pme_ctx_ctrl_token ctx_ctr;
	struct completion cb_done;
	enum pme_status cmd_status;
	u8 res_flag;
};

static void ctrl_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_ctrl_token *token)
{
	struct ctrl_op *ctrl = (struct ctrl_op *)token;
	pr_info("pme2_test_high: ctrl_cb() invoked, fd;!\n");
	ctrl->cmd_status = pme_fd_res_status(fd);
	ctrl->res_flag = pme_fd_res_flags(fd);
	complete(&ctrl->cb_done);
}

static int exclusive_inc(struct file *fp, struct db_session *db)
{
	int ret;

	BUG_ON(!db);
	BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
	spin_lock(&exclusive_lock);
	ret = pme_ctx_exclusive_inc(&db->ctx,
			(PME_CTX_OP_WAIT | PME_CTX_OP_WAIT_INT));
	if (!ret)
		db->exclusive_counter++;
	spin_unlock(&exclusive_lock);
	return ret;
}

static int exclusive_dec(struct file *fp, struct db_session *db)
{
	int ret = 0;

	BUG_ON(!db);
	BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
	spin_lock(&exclusive_lock);
	if (!db->exclusive_counter) {
		PMEPRERR("exclusivity counter already zero\n");
		ret = -EINVAL;
	} else {
		pme_ctx_exclusive_dec(&db->ctx);
		db->exclusive_counter--;
	}
	spin_unlock(&exclusive_lock);
	return ret;
}

static int execute_cmd(struct file *fp, struct db_session *db,
			unsigned long arg)
{
	int ret = 0;
	struct cmd_token token;
	/* The kernels copy of the user op structure */
	struct pme_db kernel_op;
	struct qm_sg_entry tx_comp[2];
	struct qm_fd tx_fd;
	void *tx_data = NULL;
	void *rx_data = NULL;
	u32 src_sz, dst_sz;
	dma_addr_t dma_addr;

	memset(&token, 0, sizeof(struct cmd_token));
	memset(tx_comp, 0, sizeof(tx_comp));
	memset(&tx_fd, 0, sizeof(struct qm_fd));
	init_completion(&token.cb_done);

	/* Copy the command to kernel space */
	if (copy_from_user(&kernel_op, (struct pme_db __user *)arg,
			sizeof(struct pme_db)))
		return -EFAULT;
	/* Copy the input */
	PMEPRINFO("Received User Space Contiguous mem \n");
	PMEPRINFO("length = %d \n", kernel_op.input.size);
	tx_data = kmalloc(kernel_op.input.size, GFP_KERNEL);
	if (!tx_data) {
		PMEPRERR("Err alloc %d byte \n", kernel_op.input.size);
		return -ENOMEM;
	}
	PMEPRINFO("kmalloc tx %p\n", tx_data);

	if (copy_from_user(tx_data,
			(void __user *)kernel_op.input.data,
			kernel_op.input.size)) {
		PMEPRERR("Error copying contigous user data \n");
		ret = -EFAULT;
		goto free_tx_data;
	}
	PMEPRINFO("Copied contiguous user data\n");

	/* Setup input frame */
	tx_comp[INPUT_FRM].final = 1;
	tx_comp[INPUT_FRM].length = kernel_op.input.size;
	dma_addr = pme_map(tx_data);
	if (pme_map_error(dma_addr)) {
		PMEPRERR("Error pme_map_error\n");
		ret = -EIO;
		goto free_tx_data;
	}
	set_sg_addr(&tx_comp[INPUT_FRM], dma_addr);
	/* setup output frame, if output is expected */
	if (kernel_op.output.size) {
		PMEPRINFO("expect output %d\n", kernel_op.output.size);
		rx_data = kmalloc(kernel_op.output.size, GFP_KERNEL);
		if (!rx_data) {
			PMEPRERR("Err alloc %d byte", kernel_op.output.size);
			ret = -ENOMEM;
			goto unmap_input_frame;
		}
		PMEPRINFO("kmalloc rx %p, size %d\n", rx_data,
				kernel_op.output.size);
		/* Setup output frame */
		tx_comp[OUTPUT_FRM].length = kernel_op.output.size;
		dma_addr = pme_map(rx_data);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto comp_frame_free_rx;
		}
		set_sg_addr(&tx_comp[OUTPUT_FRM], dma_addr);
		tx_fd.format = qm_fd_compound;
		/* Build compound frame */
		dma_addr = pme_map(tx_comp);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto comp_frame_unmap_output;
		}
		set_fd_addr(&tx_fd, dma_addr);
	} else {
		tx_fd.format = qm_fd_sg_big;
		tx_fd.length29 = kernel_op.input.size;
		/* Build sg frame */
		dma_addr = pme_map(&tx_comp[INPUT_FRM]);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto unmap_input_frame;
		}
		set_fd_addr(&tx_fd, dma_addr);
	}
	PMEPRINFO("About to call pme_ctx_pmtcc\n");
	ret = pme_ctx_pmtcc(&db->ctx, PME_CTX_OP_WAIT, &tx_fd,
				(struct pme_ctx_token *)&token);
	if (unlikely(ret)) {
		PMEPRINFO("pme_ctx_pmtcc error %d\n", ret);
		goto unmap_frame;
	}
	PMEPRINFO("Wait for completion\n");
	/* Wait for the command to complete */
	wait_for_completion(&token.cb_done);

	PMEPRINFO("pme2_db: process_completed_token\n");
	PMEPRINFO("pme2_db: received %d frame type\n", token.rx_fd.format);
	if (token.rx_fd.format == qm_fd_compound) {
		/* Need to copy  output */
		src_sz = tx_comp[OUTPUT_FRM].length;
		dst_sz = kernel_op.output.size;
		PMEPRINFO("pme gen %u data, have space for %u\n",
				src_sz, dst_sz);
		kernel_op.output.size = min(dst_sz, src_sz);
		/* Doesn't make sense we generated more than available space
		 * should have got truncation.
		 */
		BUG_ON(dst_sz < src_sz);
		if (copy_to_user((void __user *)kernel_op.output.data, rx_data,
				kernel_op.output.size)) {
			PMEPRERR("Error copying to user data \n");
			ret = -EFAULT;
			goto comp_frame_unmap_cf;
		}
	} else if (token.rx_fd.format == qm_fd_sg_big)
		kernel_op.output.size = 0;
	else
		panic("unexpected frame type received %d\n",
				token.rx_fd.format);

	kernel_op.flags = pme_fd_res_flags(&token.rx_fd);
	kernel_op.status = pme_fd_res_status(&token.rx_fd);
	PMEPRINFO("process_completed_token, cpy to user\n");
	/* Update the used values */
	if (unlikely(copy_to_user((struct pme_db __user *)arg, &kernel_op,
				sizeof(struct pme_db))))
		ret = -EFAULT;

unmap_frame:
	if (token.rx_fd.format == qm_fd_sg_big)
		goto single_frame_unmap_frame;

comp_frame_unmap_cf:
comp_frame_unmap_output:
comp_frame_free_rx:
	kfree(rx_data);
	goto unmap_input_frame;
single_frame_unmap_frame:
unmap_input_frame:
free_tx_data:
	kfree(tx_data);

	return ret;
}

static int execute_nop(struct file *fp, struct db_session *db)
{
	int ret = 0;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb
	};
	init_completion(&ctx_ctrl.cb_done);

	ret = pme_ctx_ctrl_nop(&db->ctx, PME_CTX_OP_WAIT|PME_CTX_OP_WAIT_INT,
			&ctx_ctrl.ctx_ctr);
	wait_for_completion(&ctx_ctrl.cb_done);
	/* pme_ctx_ctrl_nop() can be interrupted waiting for the response
	 * of the NOP. In this scenario, 0 is returned. The only way to
	 * determine that is was interrupted is to check for signal_pending()
	 */
	if (!ret && signal_pending(current))
		ret = -ERESTARTSYS;
	return ret;
}

static atomic_t sre_reset_lock = ATOMIC_INIT(1);
static int ioctl_sre_reset(unsigned long arg)
{
	struct pme_db_sre_reset reset_vals;
	int i;
	u32 srrr_val;
	int ret = 0;

	if (copy_from_user(&reset_vals, (struct pme_db_sre_reset __user *)arg,
			sizeof(struct pme_db_sre_reset)))
		return -EFAULT;
	PMEPRINFO("sre_reset: \n");
	PMEPRINFO("  rule_index = 0x%x: \n", reset_vals.rule_index);
	PMEPRINFO("  rule_increment = 0x%x: \n", reset_vals.rule_increment);
	PMEPRINFO("  rule_repetitions = 0x%x: \n", reset_vals.rule_repetitions);
	PMEPRINFO("  rule_reset_interval = 0x%x: \n",
			reset_vals.rule_reset_interval);
	PMEPRINFO("  rule_reset_priority = 0x%x: \n",
			reset_vals.rule_reset_priority);

	/* Validate ranges */
	if ((reset_vals.rule_index >= PME_PMFA_SRE_INDEX_MAX) ||
			(reset_vals.rule_increment > PME_PMFA_SRE_INC_MAX) ||
			(reset_vals.rule_repetitions >= PME_PMFA_SRE_REP_MAX) ||
			(reset_vals.rule_reset_interval >=
				PME_PMFA_SRE_INTERVAL_MAX))
		return -ERANGE;
	/* Check and make sure only one caller is present */
	if (!atomic_dec_and_test(&sre_reset_lock)) {
		/* Someone else is already in this call */
		atomic_inc(&sre_reset_lock);
		return -EBUSY;
	};
	/* All validated.  Run the command */
	for (i = 0; i < PME_SRE_RULE_VECTOR_SIZE; i++)
		pme_attr_set(pme_attr_srrv0 + i, reset_vals.rule_vector[i]);
	pme_attr_set(pme_attr_srrfi, reset_vals.rule_index);
	pme_attr_set(pme_attr_srri, reset_vals.rule_increment);
	pme_attr_set(pme_attr_srrwc,
			(0xFFF & reset_vals.rule_reset_interval) << 1 |
			(reset_vals.rule_reset_priority ? 1 : 0));
	/* Need to set SRRR last */
	pme_attr_set(pme_attr_srrr, reset_vals.rule_repetitions);
	do {
		mdelay(PME_PMFA_SRE_POLL_MS);
		ret = pme_attr_get(pme_attr_srrr, &srrr_val);
		if (!ret) {
			PMEPRCRIT("pme2: Error reading srrr\n");
			/* bail */
			break;
		}
		/* Check for error */
		else if (srrr_val & 0x10000000) {
			PMEPRERR("pme2: Error in SRRR\n");
			ret = -EIO;
		}
		PMEPRINFO("pme2: srrr count %d\n", srrr_val);
	} while (srrr_val);
	atomic_inc(&sre_reset_lock);
	return ret;
}

/**
 * fsl_pme2_db_open - open the driver
 *
 * Open the driver and prepare for requests.
 *
 * Every time an application opens the driver, we create a db_session object
 * for that file handle.
 */
static int fsl_pme2_db_open(struct inode *node, struct file *fp)
{
	int ret;
	struct db_session *db = NULL;

	db = kzalloc(sizeof(struct db_session), GFP_KERNEL);
	if (!db)
		return -ENOMEM;
	fp->private_data = db;
	db->ctx.cb = db_cb;

	ret = pme_ctx_init(&db->ctx,
			PME_CTX_FLAG_EXCLUSIVE |
			PME_CTX_FLAG_PMTCC |
			PME_CTX_FLAG_DIRECT|
			PME_CTX_FLAG_LOCAL,
			0, 4, CONFIG_FSL_PME2_DB_QOSOUT_PRIORITY, 0, NULL);
	if (ret) {
		PMEPRERR("pme_ctx_init %d \n", ret);
		goto free_data;
	}

	/* enable the context */
	ret = pme_ctx_enable(&db->ctx);
	if (ret) {
		PMEPRERR("error enabling ctx %d\n", ret);
		pme_ctx_finish(&db->ctx);
		goto free_data;
	}
	PMEPRINFO("pme2_db: Finish pme_db open %d \n", smp_processor_id());
	return 0;
free_data:
	kfree(fp->private_data);
	fp->private_data = NULL;
	return ret;
}

static int fsl_pme2_db_close(struct inode *node, struct file *fp)
{
	int ret = 0;
	struct db_session *db = fp->private_data;

	PMEPRINFO("Start pme_db close\n");
	while (db->exclusive_counter) {
		pme_ctx_exclusive_dec(&db->ctx);
		db->exclusive_counter--;
	}

	/* Disable context. */
	ret = pme_ctx_disable(&db->ctx, PME_CTX_OP_WAIT);
	if (ret)
		PMEPRCRIT("Error disabling ctx %d\n", ret);
	pme_ctx_finish(&db->ctx);
	kfree(db);
	PMEPRINFO("Finish pme_db close\n");
	return 0;
}

/* Main switch loop for ioctl operations */
static int fsl_pme2_db_ioctl(struct inode *inode, struct file *fp,
		unsigned int cmd, unsigned long arg)
{
	struct db_session *db = fp->private_data;
	int ret = 0;

	switch (cmd) {

	case PMEIO_PMTCC:
		return execute_cmd(fp, db, arg);
	case PMEIO_EXL_INC:
		return exclusive_inc(fp, db);
	case PMEIO_EXL_DEC:
		return exclusive_dec(fp, db);
	case PMEIO_EXL_GET:
		BUG_ON(!db);
		BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
		if (copy_to_user((void __user *)arg,
				&db->exclusive_counter,
				sizeof(db->exclusive_counter)))
			ret = -EFAULT;
		return ret;
	case PMEIO_NOP:
		return execute_nop(fp, db);
	case PMEIO_SRE_RESET:
		return ioctl_sre_reset(arg);
	}

	return -EINVAL;
}

static const struct file_operations fsl_pme2_db_fops = {
	.owner =	THIS_MODULE,
	.ioctl =	fsl_pme2_db_ioctl,
	.open = 	fsl_pme2_db_open,
	.release =	fsl_pme2_db_close,
};

static struct miscdevice fsl_pme2_db_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PME_DEV_DB_NODE,
	.fops = &fsl_pme2_db_fops
};

static int __init fsl_pme2_db_init(void)
{
	int err = 0;

	pr_info("Freescale pme2 db driver\n");
	if (!pme2_have_control()) {
		PMEPRERR("not on ctrl-plane\n");
		return -ENODEV;
	}
	err = misc_register(&fsl_pme2_db_dev);
	if (err) {
		PMEPRERR("cannot register device\n");
		return err;
	}
	PMEPRINFO("device %s registered\n", fsl_pme2_db_dev.name);
	return 0;
}

static void __exit fsl_pme2_db_exit(void)
{
	int err = misc_deregister(&fsl_pme2_db_dev);
	if (err) {
		PMEPRERR("Failed to deregister device %s, "
			"code %d\n", fsl_pme2_db_dev.name, err);
		return;
	}
	PMEPRINFO("device %s deregistered\n", fsl_pme2_db_dev.name);
}

module_init(fsl_pme2_db_init);
module_exit(fsl_pme2_db_exit);

MODULE_AUTHOR("Freescale Semiconductor - OTC");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 db driver");
