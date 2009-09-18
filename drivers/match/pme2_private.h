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

#include "pme2_sys.h"
#include <linux/fsl_pme.h>

#define PME2_DEBUG

#ifdef PME2_DEBUG
#define PMEPRINFO(fmt, args...) pr_info("PME2: %s: " fmt, __func__, ## args)
#else
#define PMEPRINFO(fmt, args...)
#endif

#define PMEPRERR(fmt, args...) pr_err("PME2: %s: " fmt, __func__, ## args)
#define PMEPRCRIT(fmt, args...) pr_crit("PME2: %s: " fmt, __func__, ## args)

static inline void set_fd_addr(struct qm_fd *fd, dma_addr_t addr)
{
	fd->addr_lo = lower_32_bits(addr);
	fd->addr_hi = upper_32_bits(addr);
}
static inline dma_addr_t get_fd_addr(const struct qm_fd *fd)
{
	if ((sizeof(dma_addr_t) > 4))
		return ((fd->addr_hi << 16) << 16) | fd->addr_lo;
	else
		return fd->addr_lo;
}
static inline void set_sg_addr(struct qm_sg_entry *sg, dma_addr_t addr)
{
	sg->addr_lo = lower_32_bits(addr);
	sg->addr_hi = upper_32_bits(addr);
}
static inline dma_addr_t get_sg_addr(const struct qm_sg_entry *sg)
{
	if ((sizeof(dma_addr_t) > 4))
		return ((sg->addr_hi << 16) << 16) | sg->addr_lo;
	else
		return sg->addr_lo;
}

/******************/
/* Datapath types */
/******************/

enum pme_mode {
	pme_mode_direct = 0x00,
	pme_mode_flow = 0x80
};

struct pme_context_a {
	enum pme_mode mode:8;
	u8 __reserved;
	/* Flow Context pointer (48-bit), ignored if mode==direct */
	u16 flow_hi;
	u32 flow_lo;
} __packed;

struct pme_context_b {
	u32 rbpid:8;
	u32 rfqid:24;
} __packed;

enum pme_cmd_type {
	pme_cmd_nop = 0x7,
	pme_cmd_flow_read = 0x5,	/* aka FCR */
	pme_cmd_flow_write = 0x4,	/* aka FCW */
	pme_cmd_pmtcc = 0x1,
	pme_cmd_scan = 0
};

/* This is the 32-bit frame "cmd/status" field, sent to PME */
union pme_cmd {
	struct pme_cmd_nop {
		enum pme_cmd_type cmd:3;
	} nop;
	struct pme_cmd_flow_read {
		enum pme_cmd_type cmd:3;
	} fcr;
	struct pme_cmd_flow_write {
		enum pme_cmd_type cmd:3;
		u8 __reserved:5;
		u8 flags;	/* See PME_CMD_FCW_*** */
	} __packed fcw;
	struct pme_cmd_pmtcc {
		enum pme_cmd_type cmd:3;
	} pmtcc;
	struct pme_cmd_scan {
		union {
			struct {
				enum pme_cmd_type cmd:3;
				u8 flags:5; /* See PME_CMD_SCAN_*** */
			} __packed;
		};
		u8 set;
		u16 subset;
	} __packed scan;
};

/* Hook from pme2_high to pme2_low */
struct qman_fq *slabfq_alloc(void);
void slabfq_free(struct qman_fq *fq);

/* Hook from pme2_high to pme2_ctrl */
int pme2_have_control(void);
int pme2_exclusive_set(struct qman_fq *fq);
int pme2_exclusive_unset(void);

/* Hook from pme2_low to pme2_sample_db */
void pme2_sample_db(void);

#define DECLARE_GLOBAL(name, t, mt, def, desc) \
        static t name = def; \
        module_param(name, mt, 0644); \
        MODULE_PARM_DESC(name, desc ", default: " __stringify(def));

/* Constants used by the SRE ioctl. */
#define PME_PMFA_SRE_POLL_MS		100
#define PME_PMFA_SRE_INDEX_MAX		(1 << 27)
#define PME_PMFA_SRE_INC_MAX		(1 << 12)
#define PME_PMFA_SRE_REP_MAX		(1 << 28)
#define PME_PMFA_SRE_INTERVAL_MAX	(1 << 12)

/* Encapsulations for mapping */
#define flow_map(flow) \
({ \
	struct pme_flow *__f913 = (flow); \
	pme_map(__f913); \
})

#define residue_map(residue) \
({ \
	struct pme_hw_residue *__f913 = (residue); \
	pme_map(__f913); \
})

