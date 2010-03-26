/*
 * CAAM hardware register-level view
 *
 * Copyright (c) 2008, 2009, Freescale Semiconductor, Inc.
 * All Rights Reserved
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
 *
 */

#ifndef REGS_H
#define REGS_H

#include <linux/types.h>
#include <linux/io.h>

/*
 * Architecture-specific register access methods
 *
 * CAAM's bus-addressable registers are 64 bits internally.
 * They have been wired to be safely accessible on 32-bit
 * architectures, however. Registers were organized such
 * that (a) they can be contained in 32 bits, (b) if not, then they
 * can be treated as two 32-bit entities, or finally (c) if they
 * must be treated as a single 64-bit value, then this can safely
 * be done with two 32-bit cycles.
 *
 * At the present time, the code is only written and tested for
 * a BE32 architecture (Power32), but a LE32 architecture (ARM) will
 * be ported soon, and a 64-bit Power variant is expected within the
 * architectural lifecycle of this device.
 *
 * For 32-bit operations on 64-bit values, CAAM follows the same
 * 64-bit register access conventions as it's predecessors, in that
 * writes are "triggered" by a write to the register at the numerically
 * higher address, thus, a full 64-bit write cycle requires a write
 * to the lower address, followed by a write to the higher address,
 * which will latch/execute the write cycle.
 *
 * For example, let's assume a SW reset of CAAM through the master
 * configuration register.
 * - SWRST is in bit 31 of MCFG.
 * - MCFG begins at base+0x0000.
 * - Bits 63-32 are a 32-bit word at base+0x0000 (numerically-lower)
 * - Bits 31-0 are a 32-bit word at base+0x0004 (numerically-higher)
 *
 * (and on Power, the convention is 0-31, 32-63, I know...)
 *
 * Assuming a 64-bit write to this MCFG to perform a software reset
 * would then require a write of 0 to base+0x0000, followed by a
 * write of 0x80000000 to base+0x0004, which would "execute" the
 * reset.
 *
 * Of course, since MCFG 63-32 is all zero, we could cheat and simply
 * write 0x8000000 to base+0x0004, and the reset would work fine.
 * However, since CAAM does contain some write-and-read-intended
 * 64-bit registers, this code defines 64-bit access methods for
 * the sake of internal consistency and simplicity, and so that a
 * clean transition to 64-bit is possible when it becomes necessary.
 *
 * There are limitations to this that the developer must recognize.
 * 32-bit architectures cannot enforce an atomic-64 operation,
 * Therefore:
 *
 * - On writes, since the HW is assumed to latch the cycle on the
 *   write of the higher-numeric-address word, then ordered
 *   writes work OK.
 *
 * - For reads, where a register contains a relevant value of more
 *   that 32 bits, the hardware employs logic to latch the other
 *   "half" of the data until read, ensuring an accurate value.
 *   This is of particular relevance when dealing with CAAM's
 *   performance counters.
 *
 */

#ifdef CONFIG_PPC32 /* normally BE32 */
static inline void wr_reg64(__be64 *reg, __be64 data)
{
	out_be32((__be32 *)reg, ((data & 0xffffffff00000000ull) >> 32));
	out_be32((__be32 *)reg + 1, (data & 0x00000000ffffffffull));
}

static inline __be64 rd_reg64(__be64 *reg)
{
	return (((u64)in_be32((__be32 *)reg)) << 32) |
		((u64)in_be32((__be32 *)reg + 1));
}

#define wr_reg32(reg, data) out_be32(reg, data)
#define rd_reg32(reg) in_be32(reg)
#define wr_reg16(reg, data) out_be16(reg, data)
#define rd_reg16(reg) in_be16(reg)
#else /* !CONFIG_PPC32 */
	/* TODO: define little-endian variant */
#endif


/*
 * jq_outentry
 * Represents each entry in a JobQ output ring
 */
struct jq_outentry {
	u32 *desc;	/* Pointer to completed descriptor */
	u32 jqstatus;	/* Status for completed descriptor */
};

/*
 * caam_mstraccess - Arrayed Secure Memory Partition Access
 *
 * Used to control access to 1-16 SM partitions per Job Queue
 */
struct caam_mstraccess {
	__be32 rsvd1;
	__be32 mstrperm;   /* Master Access Permissions/Partition  */
	__be32 rsvd2;
	__be32 mstrid;     /* Allowed Master ID/Partition          */
};

/*
 * caam_perfmon - Performance Monitor/Secure Memory Status/
 *                CAAM Global Status/Component Version IDs
 *
 * Spans f00-fff wherever instantiated
 */
struct caam_perfmon {
	/* Performance Monitor Registers                        f00-f38 */
	__be64 req_dequeued;   /* Dequeued Requests         */
	__be64 ob_enc_req;     /* Outbound Encrypt Requests */
	__be64 ib_dec_req;     /* Inbound Decrypt Requests  */
	__be64 ob_enc_bytes;   /* Outbound Bytes Encrypted  */
	__be64 ob_prot_bytes;  /* Outbound Bytes Protected  */
	__be64 ib_dec_bytes;   /* Inbound Bytes Decrypted   */
	__be64 ib_valid_bytes; /* Inbound Bytes Validated   */
	__be64 rsvd[15];

	/* Secure Memory                                        fb0-fbf */
	__be64 smstatus;     /* SMSTA - Secure Memory Status      */
	__be64 smpartowner;  /* SMPO  - SM Partition Owner        */

	/* CAAM Global Status                                   fc0-fd7 */
	__be64 faultaddr;    /* CFAR  - CAAM Fault Address        */
	__be32 rsvd1;
	__be32 faultdetail;  /* CFADR - CAAM Fault Addr Detail    */
	__be32 rsvd2;
	__be32 status;       /* CSTA  - CAAM Status               */

	/* Component ID Space                                   fd8-fff */
	__be64 sm_id;        /* SMVID - Secure Memory Version ID  */
	__be64 ccb_id;       /* CCBVID - CCB Version ID           */
	__be64 cha_id;       /* CHAVID - CHA Version ID           */
	__be64 rtic_id;      /* RVID - RTIC Version ID            */
	__be64 caam_id;      /* CAAMVID - CAAM Version ID         */
};


/* Master ID for DMA configuration */
struct masterid {
	__be32 rsvd1;
	__be32 midr;	/* master ID, JobQ or RTIC */
};

/* Partition ID for DMA configuration */
struct partid {
	__be32 rsvd1;
	__be32 pidr;	/* partition ID, DECO */
};

/* RNG test mode (replicated twice in some configurations) */
/* Padded out to 0x100 */
struct rngtst {
	__be32 mode;		/* Test mode */
	__be32 rsvd1[3];
	__be32 reset;		/* Test reset control */
	__be32 rsvd2[3];
	__be32 status;		/* Test status */
	__be32 rsvd3;
	__be32 errstat;	/* Test error status */
	__be32 rsvd4;
	__be32 errctl;		/* Test error control */
	__be32 rsvd5;
	__be32 entropy;	/* Test entropy */
	__be32 rsvd6[15];
	__be32 verifctl;	/* Test verification control */
	__be32 rsvd7;
	__be32 verifstat;	/* Test verification status */
	__be32 rsvd8;
	__be32 verifdata;	/* Test verification data */
	__be32 rsvd9;
	__be32 xkey;		/* Test XKEY */
	__be32 rsvd10;
	__be32 oscctctl;	/* Test oscillator counter control */
	__be32 rsvd11;
	__be32 oscct;		/* Test oscillator counter */
	__be32 rsvd12;
	__be32 oscctstat;	/* Test oscillator counter status */
	__be32 rsvd13[2];
	__be32 ofifo[4];	/* Test output FIFO */
	__be32 rsvd14[15];
};

/*
 * caam_ctrl - basic core configuration
 * starts base + 0x0000 padded out to 0x1000
 */

#define KEK_KEY_SIZE		8
#define TKEK_KEY_SIZE		8
#define TDSK_KEY_SIZE		8

struct caam_ctrl {
	/* Basic Configuration Section                          000-02f */
	/* Read/Writable                                                */
	__be32 rsvd1;
	__be32 mcr;		/* MCFG      Master Config Register  */
	__be32 rsvd2[2];

	/* Bus Access Configuration Section                     090-0c7 */
	/* Read/Writable                                                */
	struct masterid jq_mid[4];	/* JQ0MIDx - 1 per queue */
	__be32 rsvd3[12];
	struct masterid rtic_mid[4];	/* RTICMID   RTIC Bus Master ID */
	__be32 rsvd4[7];
	__be32 deco_rq;			/* DECORR    DECO Request */
	struct partid deco_mid[5];	/* DECOMIDx - 1 per DECO */
	__be32 rsvd5[206];

	/* Key Encryption/Decryption Configuration              400-47f */
	/* Read/Writable only while in Non-secure mode                  */
	__be32 kek[KEK_KEY_SIZE];	/* KEK - Key Encryption Key */
	__be32 tkek[TKEK_KEY_SIZE];	/* TKEK - Trusted Key Encryption Key */
	__be32 tdsk[TDSK_KEY_SIZE];	/* TDSK - Trusted Desc Signing Key */
	__be32 rsvd6[104];

	/* RNG Test/Verification/Debug Access                   600-6b7 */
	/* (Useful in Test/Debug modes only...)                         */
	struct rngtst rtst[2];

	/* LIODN Accessibility Mapping                          800-9ff */
	__be16 pidmap[64];   /* CPIDMR - PID/LIODN Mapping        */
	__be32 rsvd7[416];

	/* Performance Monitor                                  f00-fff */
	struct caam_perfmon perfmon;
};

/*
 * Controller master config register defs
 */
#define MCFGR_SWRESET		0x80000000 /* software reset */
#define MCFGR_WDENABLE		0x40000000 /* DECO watchdog enable */
#define MCFGR_WDFAIL		0x20000000 /* DECO watchdog force-fail */
#define MCFGR_DMA_RESET		0x10000000
#define MCFGR_LONG_PTR		0x00010000 /* Use >32-bit desc addressing */

/* AXI read cache control */
#define MCFGR_ARCACHE_SHIFT	12
#define MCFGR_ARCACHE_MASK	(0xf << MCFGR_ARCACHE_SHIFT)

/* AXI write cache control */
#define MCFGR_AWCACHE_SHIFT	8
#define MCFGR_AWCACHE_MASK	(0xf << MCFGR_AWCACHE_SHIFT)

/* AXI pipeline depth */
#define MCFGR_AXIPIPE_SHIFT	4
#define MCFGR_AXIPIPE_MASK	(0xf << MCFGR_AXIPIPE_SHIFT)

#define MCFGR_AXIPRI		0x00000008 /* Assert AXI priority sideband */
#define MCFGR_BURST_64		0x00000001 /* Max burst size */

/*
 * caam_job_queue - direct job queue setup
 * 1-4 possible per instantiation, base + 1000/2000/3000/4000
 * Padded out to 0x1000
 */
struct caam_job_queue {
	/* Input ring */
	__be64 inpring_base;	/* Input desc ring baseaddr */
	__be32 rsvd1;
	__be32 inpring_size;	/* Input ring size */
	__be32 rsvd2;
	__be32 inpring_avail;	/* Input ring room remaining */
	__be32 rsvd3;
	__be32 inpring_jobadd;	/* Input ring jobs added */

	/* Output Ring */
	__be64 outring_base;	/* Output status ring base addr */
	__be32 rsvd4;
	__be32 outring_size;	/* Output ring size */
	__be32 rsvd5;
	__be32 outring_rmvd;	/* Output ring jobs removed */
	__be32 rsvd6;
	__be32 outring_used;	/* Output ring slots full */

	/* Status/Configuration */
	__be32 rsvd7;
	__be32 jqoutstatus;	/* JobQ output status */
	__be32 rsvd8;
	__be32 jqintstatus;	/* JobQ interrupt status */
	__be32 qconfig_hi;	/* Queue configuration */
	__be32 qconfig_lo;

	/* Indices. CAAM maintains as "heads" of each queue */
	__be32 rsvd9;
	__be32 inp_rdidx;	/* Input ring read index */
	__be32 rsvd10;
	__be32 out_wtidx;	/* Output ring write index */

	/* Command/control */
	__be32 rsvd11;
	__be32 jqcommand;	/* JobQ command */

	__be32 rsvd12[128];

	/* Secure Memory Access Configuration */
	__be32 rsvd13;
	__be32 smemcmd;         /* Secure Memory Command */
	__be32 rsvd14;
	__be32 smemstatus;      /* Secure Memory Command Status */
	struct caam_mstraccess access[16]; /* SM Partition Access Perms */

	__be32 rsvd15[800];
};

#define JQ_RINGSIZE_MASK	0x03ff
/*
 * jqstatus - Job Queue Output Status
 * All values in lo word
 * Also note, same values written out as status through QI
 * in the command/status field of a frame descriptor
 */
#define JQSTA_SSRC_SHIFT            28
#define JQSTA_SSRC_MASK             0xf0000000

#define JQSTA_SSRC_NONE             0x00000000
#define JQSTA_SSRC_CCB_ERROR        0x20000000
#define JQSTA_SSRC_JUMP_HALT_USER   0x30000000
#define JQSTA_SSRC_DECO             0x40000000
#define JQSTA_SSRC_JQERROR          0x60000000
#define JQSTA_SSRC_JUMP_HALT_CC     0x70000000

#define JQSTA_DECOERR_JUMP          0x08000000
#define JQSTA_DECOERR_INDEX_SHIFT   8
#define JQSTA_DECOERR_INDEX_MASK    0xff00
#define JQSTA_DECOERR_ERROR_MASK    0x00ff

#define JQSTA_DECOERR_NONE          0x00
#define JQSTA_DECOERR_LINKLEN       0x01
#define JQSTA_DECOERR_LINKPTR       0x02
#define JQSTA_DECOERR_JQCTRL        0x03
#define JQSTA_DECOERR_DESCCMD       0x04
#define JQSTA_DECOERR_ORDER         0x05
#define JQSTA_DECOERR_KEYCMD        0x06
#define JQSTA_DECOERR_LOADCMD       0x07
#define JQSTA_DECOERR_STORECMD      0x08
#define JQSTA_DECOERR_OPCMD         0x09
#define JQSTA_DECOERR_FIFOLDCMD     0x0a
#define JQSTA_DECOERR_FIFOSTCMD     0x0b
#define JQSTA_DECOERR_MOVECMD       0x0c
#define JQSTA_DECOERR_JUMPCMD       0x0d
#define JQSTA_DECOERR_MATHCMD       0x0e
#define JQSTA_DECOERR_SHASHCMD      0x0f
#define JQSTA_DECOERR_SEQCMD        0x10
#define JQSTA_DECOERR_DECOINTERNAL  0x11
#define JQSTA_DECOERR_SHDESCHDR     0x12
#define JQSTA_DECOERR_HDRLEN        0x13
#define JQSTA_DECOERR_BURSTER       0x14
#define JQSTA_DECOERR_DESCSIGNATURE 0x15
#define JQSTA_DECOERR_DMA           0x16
#define JQSTA_DECOERR_BURSTFIFO     0x17
#define JQSTA_DECOERR_JQRESET       0x1a
#define JQSTA_DECOERR_JOBFAIL       0x1b
#define JQSTA_DECOERR_DNRERR        0x80
#define JQSTA_DECOERR_UNDEFPCL      0x81
#define JQSTA_DECOERR_PDBERR        0x82
#define JQSTA_DECOERR_ANRPLY_LATE   0x83
#define JQSTA_DECOERR_ANRPLY_REPLAY 0x84
#define JQSTA_DECOERR_SEQOVF        0x85
#define JQSTA_DECOERR_INVSIGN       0x86
#define JQSTA_DECOERR_DSASIGN       0x87

#define JQSTA_CCBERR_JUMP           0x08000000
#define JQSTA_CCBERR_INDEX_MASK     0xff00
#define JQSTA_CCBERR_INDEX_SHIFT    8
#define JQSTA_CCBERR_CHAID_MASK     0x00f0
#define JQSTA_CCBERR_CHAID_SHIFT    4
#define JQSTA_CCBERR_ERRID_MASK     0x000f

#define JQSTA_CCBERR_CHAID_AES      (0x01 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_DES      (0x02 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_ARC4     (0x03 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_MD       (0x04 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_RNG      (0x05 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_SNOW     (0x06 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_KASUMI   (0x07 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_PK       (0x08 << JQSTA_CCBERR_CHAID_SHIFT)
#define JQSTA_CCBERR_CHAID_CRC      (0x09 << JQSTA_CCBERR_CHAID_SHIFT)

#define JQSTA_CCBERR_ERRID_NONE     0x00
#define JQSTA_CCBERR_ERRID_MODE     0x01
#define JQSTA_CCBERR_ERRID_DATASIZ  0x02
#define JQSTA_CCBERR_ERRID_KEYSIZ   0x03
#define JQSTA_CCBERR_ERRID_PKAMEMSZ 0x04
#define JQSTA_CCBERR_ERRID_PKBMEMSZ 0x05
#define JQSTA_CCBERR_ERRID_SEQUENCE 0x06
#define JQSTA_CCBERR_ERRID_PKDIVZRO 0x07
#define JQSTA_CCBERR_ERRID_PKMODEVN 0x08
#define JQSTA_CCBERR_ERRID_KEYPARIT 0x09
#define JQSTA_CCBERR_ERRID_ICVCHK   0x0a
#define JQSTA_CCBERR_ERRID_HARDWARE 0x0b
#define JQSTA_CCBERR_ERRID_CCMAAD   0x0c
#define JQSTA_CCBERR_ERRID_INVCHA   0x0f

#define JQINT_ERR_INDEX_MASK        0x3fff0000
#define JQINT_ERR_INDEX_SHIFT       16
#define JQINT_ERR_TYPE_MASK         0xf00
#define JQINT_ERR_TYPE_SHIFT        8
#define JQINT_ERR_HALT_MASK         0x0c
#define JQINT_ERR_HALT_SHIFT        2
#define JQINT_JQ_ERROR              0x02
#define JQINT_JQ_INT                0x01

#define JQINT_ERR_TYPE_WRITE        1
#define JQINT_ERR_TYPE_BAD_INPADDR  3
#define JQINT_ERR_TYPE_BAD_OUTADDR  4
#define JQINT_ERR_TYPE_INV_INPWRT   5
#define JQINT_ERR_TYPE_INV_OUTWRT   6
#define JQINT_ERR_TYPE_RESET        7
#define JQINT_ERR_TYPE_REMOVE_OFL   8
#define JQINT_ERR_TYPE_ADD_OFL      9

#define JQCFG_SOE		0x04
#define JQCFG_ICEN		0x02
#define JQCFG_IMSK		0x01
#define JQCFG_ICDCT_SHIFT	8
#define JQCFG_ICTT_SHIFT	16

#define JQCR_RESET                  0x01

/*
 * caam_assurance - Assurance Controller View
 * base + 0x6000 padded out to 0x1000
 */

struct rtic_element {
	__be64 address;
	__be32 rsvd;
	__be32 length;
};

struct rtic_block {
	struct rtic_element element[2];
};

struct rtic_memhash {
	__be32 memhash_be[32];
	__be32 memhash_le[32];
};

struct caam_assurance {
    /* Status/Command/Watchdog */
	__be32 rsvd1;
	__be32 status;		/* RTIC Status                     */
	__be32 rsvd2;
	__be32 cmd;		/* RTIC Command                    */
	__be32 rsvd3;
	__be32 ctrl;		/* RTIC Control                    */
	__be32 rsvd4;
	__be32 throttle;	/* RTIC Throttle                   */
	__be32 rsvd5;
	__be64 watchdog;	/* RTIC Watchdog Timer             */
	__be32 rsvd6;
	__be32 rend;		/* Endian corrections */
	__be32 rsvd7[50];

	/* Block access/configuration @ 100/110/120/130 */
	struct rtic_block memblk[4];	/* RTIC Memory Blocks A-D */
	__be32 rsvd8[32];

	/* Block hashes @ 200/300/400/500 */
	struct rtic_memhash hash[4];	/* Block hash values A-D */
	__be32 rsvd_3[640];
};

/*
 * caam_queue_if - QI configuration and control
 * starts base + 0x7000, padded out to 0x1000 long
 */

struct caam_queue_if {
	__be32 qi_control_hi;	/* QICTL  - QI Control */
	__be32 qi_control_lo;
	__be32 rsvd1;
	__be32 qi_status;	/* QISTA  - QI Status */
	__be32 qi_deq_cfg_hi;	/* QIDQC  - QI Dequeue Configuration */
	__be32 qi_deq_cfg_lo;
	__be32 qi_enq_cfg_hi;	/* QISEQC - QI Enqueue Command     */
	__be32 qi_enq_cfg_lo;
	__be32 rsvd2[1016];
};

/* QI control bits - low word */
#define QICTL_DQEN      0x01              /* Enable frame pop          */
#define QICTL_STOP      0x02              /* Stop dequeue/enqueue      */
#define QICTL_SOE       0x04              /* Stop on error             */

/* QI control bits - high word */
#define QICTL_MBSI	0x01
#define QICTL_MHWSI	0x02
#define QICTL_MWSI	0x04
#define QICTL_MDWSI	0x08
#define QICTL_CBSI	0x10		/* CtrlDataByteSwapInput     */
#define QICTL_CHWSI	0x20		/* CtrlDataHalfSwapInput     */
#define QICTL_CWSI	0x40		/* CtrlDataWordSwapInput     */
#define QICTL_CDWSI	0x80		/* CtrlDataDWordSwapInput    */
#define QICTL_MBSO	0x0100
#define QICTL_MHWSO	0x0200
#define QICTL_MWSO	0x0400
#define QICTL_MDWSO	0x0800
#define QICTL_CBSO	0x1000		/* CtrlDataByteSwapOutput    */
#define QICTL_CHWSO	0x2000		/* CtrlDataHalfSwapOutput    */
#define QICTL_CWSO	0x4000		/* CtrlDataWordSwapOutput    */
#define QICTL_CDWSO     0x8000		/* CtrlDataDWordSwapOutput   */
#define QICTL_DMBS	0x010000
#define QICTL_EPO	0x020000

/* QI status bits */
#define QISTA_PHRDERR   0x01              /* PreHeader Read Error      */
#define QISTA_CFRDERR   0x02              /* Compound Frame Read Error */
#define QISTA_OFWRERR   0x04              /* Output Frame Read Error   */
#define QISTA_BPDERR    0x08              /* Buffer Pool Depleted      */
#define QISTA_BTSERR    0x10              /* Buffer Undersize          */
#define QISTA_CFWRERR   0x20              /* Compound Frame Write Err  */
#define QISTA_STOPD     0x80000000        /* QI Stopped (see QICTL)    */

/*
 * caam_deco - descriptor controller - CHA cluster block
 *
 * Only accessible when direct DECO access is turned on
 * (done in DECORR, via MID programmed in DECOxMID
 *
 * 5 typical, base + 0x8000/9000/a000/b000
 * Padded out to 0x1000 long
 */
struct caam_deco {
	__be32 rsvd1;
	__be32 cls1_mode;	/* Class 1 Mode */
	__be32 rsvd2;
	__be32 cls1_keysize;	/* Class 1 Key Size */
	__be32 cls1_datasize_hi;	/* Class 1 Data Size */
	__be32 cls1_datasize_lo;
	__be32 rsvd3;
	__be32 cls1_icvsize;	/* Class 1 ICV size */
	__be32 rsvd4[5];
	__be32 cha_ctrl;	/* CHA control */
	__be32 rsvd5;
	__be32 irq_crtl;	/* CCB interrupt done/error/clear */
	__be32 rsvd6;
	__be32 clr_written;	/* Clear-Written */
	__be32 ccb_status_hi;	/* CCB Status */
	__be32 ccb_status_lo;
	__be32 rsvd7[3];
	__be32 aad_size;	/* Current AAD Size */
	__be32 rsvd8;
	__be32 cls1_iv_size;	/* Current Class 1 IV Size */
	__be32 rsvd9[7];
	__be32 pkha_a_size;	/* Size of PKHA A */
	__be32 rsvd10;
	__be32 pkha_b_size;	/* Size of PKHA B */
	__be32 rsvd11;
	__be32 pkha_n_size;	/* Size of PKHA N */
	__be32 rsvd12;
	__be32 pkha_e_size;	/* Size of PKHA E */
	__be32 rsvd13[24];
	__be32 cls1_ctx[16];	/* Class 1 Context @100 */
	__be32 rsvd14[48];
	__be32 cls1_key[8];	/* Class 1 Key @200 */
	__be32 rsvd15[121];
	__be32 cls2_mode;	/* Class 2 Mode */
	__be32 rsvd16;
	__be32 cls2_keysize;	/* Class 2 Key Size */
	__be32 cls2_datasize_hi;	/* Class 2 Data Size */
	__be32 cls2_datasize_lo;
	__be32 rsvd17;
	__be32 cls2_icvsize;	/* Class 2 ICV Size */
	__be32 rsvd18[56];
	__be32 cls2_ctx[18];	/* Class 2 Context @500 */
	__be32 rsvd19[46];
	__be32 cls2_key[32];	/* Class2 Key @600 */
	__be32 rsvd20[84];
	__be32 inp_infofifo;	/* Input Info FIFO @7d0 */
	__be32 rsvd21[3];
	__be32 inp_datafifo;	/* Input Data FIFO */
	__be32 rsvd22[3];
	__be64 out_datafifo;	/* Output Data FIFO */
	__be32 rsvd23[2];
	__be32 jq_ctl_hi;	/* JobQ Control Register      @800 */
	__be32 jq_ctl_lo;
	__be64 jq_descaddr;	/* JobQ Descriptor Address */
	__be32 op_status_hi;	/* DECO Operation Status */
	__be32 op_status_lo;
	__be32 rsvd24[10];
	__be64 math[4];		/* Math register */
	__be32 rsvd25[104];
	__be32 descbuf[64];	/* Descriptor buffer */
	__be32 rsvd26[320];
};

/*
 * Current top-level view of memory map is:
 *
 * 0x0000 - 0x0fff - CAAM Top-Level Control
 * 0x1000 - 0x1fff - Job Queue 0
 * 0x2000 - 0x2fff - Job Queue 1
 * 0x3000 - 0x3fff - Job Queue 2
 * 0x4000 - 0x4fff - Job Queue 3
 * 0x5000 - 0x5fff - (unused)
 * 0x6000 - 0x6fff - Assurance Controller
 * 0x7000 - 0x7fff - Queue Interface
 * 0x8000 - 0x8fff - DECO-CCB 0
 * 0x9000 - 0x9fff - DECO-CCB 1
 * 0xa000 - 0xafff - DECO-CCB 2
 * 0xb000 - 0xbfff - DECO-CCB 3
 * 0xc000 - 0xcfff - DECO-CCB 4
 *
 * caam_full describes the full register view of CAAM if useful,
 * although many configurations may choose to implement parts of
 * the register map separately, in differing privilege regions
 */
struct caam_full {
    struct caam_ctrl ctrl;
    struct caam_job_queue jq[4];
    __be64 rsvd[512];
    struct caam_assurance assure;
    struct caam_queue_if qi;
    struct caam_deco deco[5];
};

#endif /* REGS_H */
