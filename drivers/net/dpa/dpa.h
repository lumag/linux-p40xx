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

#ifndef __DPA_H
#define __DPA_H

#include <linux/ethtool.h>	/* struct ethtool_ops */
#include <linux/netdevice.h>
#include <linux/list.h>		/* struct list_head */
#include <linux/workqueue.h>	/* struct work_struct */
#include <linux/skbuff.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/dcache.h>	/* struct dentry */
#endif

#include "linux/fsl_qman.h"	/* struct qman_fq */

#include "dpa-common.h"

#include "mac.h"		/* struct mac_device */

struct pcd_range {
	uint32_t			 base;
	uint32_t			 count;
};

struct dpa_percpu_priv_s {
	struct net_device	*net_dev;
	struct work_struct	 fd_work;
	struct list_head	 fd_list[2];
	struct list_head	 free_list;
	struct sk_buff_head	 rx_recycle;
#ifdef CONFIG_DEBUG_FS
	size_t			 count[2], total[2], max[2], hwi[2], swi;
#endif
};

struct dpa_priv_s {
	struct list_head	 dpa_bp_list;

	uint16_t		 channel;
	struct list_head	 dpa_fq_list[2];
	struct qman_fq		*egress_fqs[8];

	int					 num;
	struct pcd_range	 ranges[4];

	struct mac_device	*mac_dev;

	struct dpa_percpu_priv_s	*percpu_priv;
#ifdef CONFIG_DEBUG_FS
	struct dentry			*debugfs_file;
#endif

	uint32_t		 msg_enable;	/* net_device message level */
};

extern const struct ethtool_ops dpa_ethtool_ops;

#endif	/* __DPA_H */
