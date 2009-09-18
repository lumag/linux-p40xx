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
#include "pme2_regs.h"

#define DEFAULT_PDSR_SZ (CONFIG_FSL_PME2_PDSRSIZE << 7)
#define DEFAULT_SRE_SZ  (CONFIG_FSL_PME2_SRESIZE << 5)
#define PDSR_TBL_ALIGN  (1 << 7)
#define SRE_TBL_ALIGN   (1 << 5)
#define DEFAULT_SRFCC   400

/* Defaults */
#define DEFAULT_DEC0_MTE   0x3FFF
#define DEFAULT_DLC_MPM    0xFFFF
#define DEFAULT_DLC_MPE    0xFFFF
/* Boot parameters */
DECLARE_GLOBAL(max_test_line_per_pat, unsigned int, uint,
		DEFAULT_DEC0_MTE,
		"Maximum allowed Test Line Executions per pattern, "
		"scaled by a factor of 8");
DECLARE_GLOBAL(max_pat_eval_per_sui, unsigned int, uint,
		DEFAULT_DLC_MPE,
		"Maximum Pattern Evaluations per SUI, scaled by a factor of 8")
DECLARE_GLOBAL(max_pat_matches_per_sui, unsigned int, uint,
		DEFAULT_DLC_MPM,
		"Maximum Pattern Matches per SUI");
/* SRE */
DECLARE_GLOBAL(sre_rule_num, unsigned int, uint,
		CONFIG_FSL_PME2_SRE_CNR,
		"Configured Number of Stateful Rules");
DECLARE_GLOBAL(sre_session_ctx_size, unsigned int, uint,
		1 << CONFIG_FSL_PME2_SRE_CTX_SIZE_PER_SESSION,
		"SRE Context Size per Session");

/************
 * Section 1
 ************
 * This code is called during kernel early-boot and could never be made
 * loadable.
 */
static dma_addr_t dxe_a, sre_a;
static size_t dxe_sz = DEFAULT_PDSR_SZ, sre_sz = DEFAULT_SRE_SZ;

/* Parse the <name> property to extract the memory location and size and
 * lmb_reserve() it. If it isn't supplied, lmb_alloc() the default size. */
static __init int parse_mem_property(struct device_node *node, const char *name,
			dma_addr_t *addr, size_t *sz, u64 align, int zero)
{
	const u32 *pint;
	int ret;

	pint = of_get_property(node, name, &ret);
	if (!pint || (ret != 16)) {
		pr_info("pme: No %s property '%s', using lmb_alloc(0x%08x)\n",
				node->full_name, name, *sz);
		*addr = lmb_alloc(*sz, align);
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
		return 0;
	}
	pr_info("pme: Using %s property '%s'\n", node->full_name, name);
	/* Props are 64-bit, but dma_addr_t is (currently) 32-bit */
	BUG_ON(sizeof(*addr) != 4);
	if(pint[0] || pint[2]) {
		pr_err("pme: Invalid number of dt properties\n");
		return -EINVAL;
	}
	*addr = pint[1];
	if((u64)*addr & (align - 1)) {
		pr_err("pme: Invalid alignment, address %016llx\n",(u64)*addr);
		return -EINVAL;
	}
	*sz = pint[3];
	/* Keep things simple, it's either all in the DRAM range or it's all
	 * outside. */
	if (*addr < lmb_end_of_DRAM()) {
		if ((u64)*addr + (u64)*sz > lmb_end_of_DRAM()){
			pr_err("pme: outside DRAM range\n");
			return -EINVAL;
		}
		if (lmb_reserve(*addr, *sz) < 0) {
			pr_err("pme: Failed to reserve %s\n", name);
			return -ENOMEM;
		}
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
	} else {
		/* map as cacheable, non-guarded */
		void *tmpp = ioremap_flags(*addr, *sz, 0);
		if (zero)
			memset(tmpp, 0, *sz);
		iounmap(tmpp);
	}
	return 0;
}

/* No errors/interrupts. Physical addresses are assumed <= 32bits. */
static int __init fsl_pme2_init(struct device_node *node)
{
	int ret = 0;

	/* Check if pdsr memory already allocated */
	if (dxe_a) {
		pr_err("pme: Error fsl_pme2_init already done\n");
		return -EINVAL;
	}
	ret = parse_mem_property(node, "fsl,pme-pdsr", &dxe_a, &dxe_sz,
			PDSR_TBL_ALIGN, 0);
	if (ret)
		return ret;
	ret = parse_mem_property(node, "fsl,pme-sre", &sre_a, &sre_sz,
			SRE_TBL_ALIGN, 0);
	return ret;
}

__init void pme2_init_early(void)
{
	struct device_node *dn;
	int ret;
	for_each_compatible_node(dn, NULL, "fsl,pme") {
		ret = fsl_pme2_init(dn);
		if (ret)
			pr_err("pme: Error fsl_pme2_init\n");
	}
}

/************
 * Section 2
 ************
 * This code is called during driver initialisation. It doesn't do anything with
 * the device-tree entries nor the PME device, it simply creates the sysfs stuff
 * and gives the user something to hold. This could be made loadable, if there
 * was any benefit to doing so - but as the device is already "bound" by static
 * code, there's little point to hiding the fact.
 */

MODULE_AUTHOR("Geoff Thorpe");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 (p4080) device control");

/* Opaque pointer target used to represent the PME CCSR map, ... */
struct pme;

/* ... and the instance of it. */
static struct pme *global_pme;
static int pme_err_irq;

static inline void __pme_out(struct pme *p, u32 offset, u32 val)
{
	u32 __iomem *regs = (void *)p;
	out_be32(regs + (offset >> 2), val);
}
#define pme_out(p, r, v) __pme_out(p, PME_REG_##r, v)
static inline u32 __pme_in(struct pme *p, u32 offset)
{
	u32 __iomem *regs = (void *)p;
	return in_be32(regs + (offset >> 2));
}
#define pme_in(p, r) __pme_in(p, PME_REG_##r)

#define PME_EFQC(en, fq) \
	({ \
		/* Assume a default delay of 64 cycles */ \
		u8 __i419 = 0x1; \
		u32 __fq419 = (fq) & 0x00ffffff; \
		((en) ? 0x80000000 : 0) | (__i419 << 28) | __fq419; \
	})

#define PME_FACONF_ENABLE   0x00000002
#define PME_FACONF_RESET    0x00000001

static inline struct pme *pme_create(void *regs)
{
	struct pme *res = (struct pme *)regs;
	pme_out(res, FACONF, 0);
	pme_out(res, EFQC, PME_EFQC(0, 0));
	pme_out(res, FACONF, PME_FACONF_ENABLE);
	/* TODO: these coherency settings for PMFA, DXE, and SRE force all
	 * transactions to snoop, as the kernel does not yet support flushing in
	 * dma_map_***() APIs (ie. h/w can not treat otherwise coherent memory
	 * in a non-coherent manner, temporarily or otherwise). When the kernel
	 * supports this, we should tune these settings back to;
	 *     FAMCR = 0x00010001
	 *      DMCR = 0x00000000
	 *      SMCR = 0x00000000
	 */
	pme_out(res, FAMCR, 0x01010101);
	pme_out(res, DMCR, 0x00000001);
	pme_out(res, SMCR, 0x00000211);
	return res;
}

/* pme stats accumulator work */
static int pme_stat_get(enum pme_attr *stat, u64 *value, int reset);
static void accumulator_update(struct work_struct *work);
static void accumulator_update_interval(u32 interval);
static DECLARE_DELAYED_WORK(accumulator_work, accumulator_update);
u32 pme_stat_interval = CONFIG_FSL_PME2_STAT_ACCUMULATOR_UPDATE_INTERVAL;
#define MAX_ACCUMULATOR_INTERVAL 10000
#define PME_SBE_ERR 0x01000000
#define PME_DBE_ERR 0x00080000
#define PME_PME_ERR 0x00000100
#define PME_ALL_ERR (PME_SBE_ERR | PME_DBE_ERR | PME_PME_ERR)

static ssize_t pme_generic_store(const char *buf, size_t count,
				enum pme_attr attr)
{
	unsigned long val;
	size_t ret;
	if (strict_strtoul(buf, 0, &val)) {
		pr_err("pme: invalid input %s\n",buf);
		return -EINVAL;
	}
	ret = pme_attr_set(attr, val);
	if (ret) {
		pr_err("pme: attr_set err attr=%u, val=%lu\n", attr, val);
		return ret;
	}
	return count;
}

static ssize_t pme_generic_show(char *buf, enum pme_attr attr, const char *fmt)
{
	u32 data;
	int ret;

	ret =  pme_attr_get(attr, &data);
	if (!ret)
		return snprintf(buf, PAGE_SIZE, fmt, data);
	else
		return ret;
}

static ssize_t pme_generic_stat_show(char *buf, enum pme_attr attr)
{
	u64 data = 0;
	int ret = 0;

	ret = pme_stat_get(&attr, &data, 0);
	if (!ret)
		return snprintf(buf, PAGE_SIZE, "%llu\n", data);
	else
		return ret;
}

static ssize_t pme_generic_stat_store(const char *buf, size_t count,
				enum pme_attr attr)
{
	unsigned long val;
	u64 data = 0;
	size_t ret = 0;
	if (strict_strtoul(buf, 0, &val)) {
		pr_err("pme: invalid input %s\n", buf);
		return -EINVAL;
	}
	if (val) {
		pr_err("pme: invalid input %s\n", buf);
		return -EINVAL;
	}
	ret = pme_stat_get(&attr, &data, 1);
	return count;
}

#define PME_GENERIC_ATTR(pme_attr, perm, showhex) \
static ssize_t pme_store_##pme_attr(struct device_driver *pme, const char *buf,\
					size_t count) \
{ \
	return pme_generic_store(buf, count, pme_attr_##pme_attr);\
} \
static ssize_t pme_show_##pme_attr(struct device_driver *pme, char *buf) \
{ \
	return pme_generic_show(buf, pme_attr_##pme_attr, showhex);\
} \
static DRIVER_ATTR( pme_attr, perm, pme_show_##pme_attr, pme_store_##pme_attr);

#define PME_GENERIC_BSC_ATTR(bsc_id, perm, showhex) \
static ssize_t pme_store_pme_attr_bsc_##bsc_id(struct device_driver *pme,\
					const char *buf, size_t count) \
{ \
	return pme_generic_store(buf, count, pme_attr_bsc(bsc_id));\
} \
static ssize_t pme_show_pme_attr_bsc_##bsc_id(struct device_driver *pme,\
						char *buf) \
{ \
	return pme_generic_show(buf, pme_attr_bsc(bsc_id), showhex);\
} \
static DRIVER_ATTR(bsc_##bsc_id, perm, pme_show_pme_attr_bsc_##bsc_id, \
			pme_store_pme_attr_bsc_##bsc_id);


#define PME_GENERIC_STAT_ATTR(pme_attr, perm) \
static ssize_t pme_store_##pme_attr(struct device_driver *pme, const char *buf,\
					size_t count) \
{ \
	return pme_generic_stat_store(buf, count, pme_attr_##pme_attr);\
} \
static ssize_t pme_show_##pme_attr(struct device_driver *pme, char *buf) \
{ \
	return pme_generic_stat_show(buf, pme_attr_##pme_attr);\
} \
static DRIVER_ATTR(pme_attr, perm, pme_show_##pme_attr, pme_store_##pme_attr);

static ssize_t pme_store_update_interval(struct device_driver *pme,
		const char *buf, size_t count)
{
	unsigned long val;

	if (!pme2_have_control()) {
		PMEPRERR("not on ctrl-plane\n");
		return -ENODEV;
	}
	if (strict_strtoul(buf, 0, &val)) {
		pr_err("pme: invalid input %s\n", buf);
		return -EINVAL;
	}
	if (val > MAX_ACCUMULATOR_INTERVAL) {
		pr_err("pme: invalid input %s\n", buf);
		return -ERANGE;
	}

	accumulator_update_interval(val);
	return count;
}
static ssize_t pme_show_update_interval(struct device_driver *pme, char *buf)
{
	if (!pme2_have_control())
		return -ENODEV;
	return snprintf(buf, PAGE_SIZE, "%u\n", pme_stat_interval);
}
static DRIVER_ATTR(update_interval, (S_IRUSR | S_IWUSR),
		pme_show_update_interval, pme_store_update_interval);

#define FMT_0HEX "0x%08x\n"
#define FMT_HEX  "0x%x\n"
#define FMT_DEC  "%u\n"
#define PRIV_RO  S_IRUSR
#define PRIV_RW  (S_IRUSR | S_IWUSR)

/* Register Interfaces */
/* read-write; */
PME_GENERIC_ATTR(efqc_int, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(sw_db, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(dmcr, PRIV_RW, FMT_0HEX);
PME_GENERIC_ATTR(smcr, PRIV_RW, FMT_0HEX);
PME_GENERIC_ATTR(famcr, PRIV_RW, FMT_0HEX);
PME_GENERIC_ATTR(kvlts, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(max_chain_length, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(pattern_range_counter_idx, PRIV_RW, FMT_0HEX);
PME_GENERIC_ATTR(pattern_range_counter_mask, PRIV_RW, FMT_0HEX);
PME_GENERIC_ATTR(max_allowed_test_line_per_pattern, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(max_pattern_matches_per_sui, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(max_pattern_evaluations_per_sui, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(report_length_limit, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(end_of_simple_sui_report, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(aim, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(end_of_sui_reaction_ptr, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(sre_pscl, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(sre_max_block_num, PRIV_RW, FMT_DEC);
PME_GENERIC_ATTR(sre_max_instruction_limit, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(0, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(1, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(2, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(3, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(4, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(5, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(6, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(7, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(8, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(9, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(10, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(11, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(12, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(13, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(14, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(15, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(16, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(17, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(18, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(19, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(20, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(21, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(22, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(23, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(24, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(25, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(26, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(27, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(28, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(29, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(30, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(31, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(32, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(33, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(34, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(35, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(36, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(37, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(38, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(39, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(40, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(41, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(42, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(43, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(44, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(45, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(46, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(47, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(48, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(49, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(50, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(51, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(52, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(53, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(54, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(55, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(56, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(57, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(58, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(59, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(60, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(61, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(62, PRIV_RW, FMT_DEC);
PME_GENERIC_BSC_ATTR(63, PRIV_RW, FMT_DEC);

/* read-only; */
PME_GENERIC_ATTR(max_pdsr_index, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(sre_context_size, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(sre_rule_num, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(sre_session_ctx_num, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(sre_max_index_size, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(sre_max_offset_ctrl, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(src_id, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(liodnr, PRIV_RO, FMT_DEC);
PME_GENERIC_ATTR(rev1, PRIV_RO, FMT_0HEX);
PME_GENERIC_ATTR(rev2, PRIV_RO, FMT_0HEX);
PME_GENERIC_ATTR(isr, PRIV_RO, FMT_0HEX);

/* Stats */
PME_GENERIC_STAT_ATTR(trunci, PRIV_RW);
PME_GENERIC_STAT_ATTR(rbc, PRIV_RW);
PME_GENERIC_STAT_ATTR(tbt0ecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(tbt1ecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(vlt0ecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(vlt1ecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(cmecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(dxcmecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(dxemecc1ec, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnib, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnis, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnth1, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnth2, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnthv, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnths, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnch, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnpm, PRIV_RW);
PME_GENERIC_STAT_ATTR(stns1m, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnpmr, PRIV_RW);
PME_GENERIC_STAT_ATTR(stndsr, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnesr, PRIV_RW);
PME_GENERIC_STAT_ATTR(stns1r, PRIV_RW);
PME_GENERIC_STAT_ATTR(stnob, PRIV_RW);
PME_GENERIC_STAT_ATTR(mia_byc, PRIV_RW);
PME_GENERIC_STAT_ATTR(mia_blc, PRIV_RW);

static struct attribute *pme_drv_attrs[] = {
	&driver_attr_efqc_int.attr,
	&driver_attr_sw_db.attr,
	&driver_attr_dmcr.attr,
	&driver_attr_smcr.attr,
	&driver_attr_famcr.attr,
	&driver_attr_kvlts.attr,
	&driver_attr_max_chain_length.attr,
	&driver_attr_pattern_range_counter_idx.attr,
	&driver_attr_pattern_range_counter_mask.attr,
	&driver_attr_max_allowed_test_line_per_pattern.attr,
	&driver_attr_max_pdsr_index.attr,
	&driver_attr_max_pattern_matches_per_sui.attr,
	&driver_attr_max_pattern_evaluations_per_sui.attr,
	&driver_attr_report_length_limit.attr,
	&driver_attr_end_of_simple_sui_report.attr,
	&driver_attr_aim.attr,
	&driver_attr_sre_context_size.attr,
	&driver_attr_sre_rule_num.attr,
	&driver_attr_sre_session_ctx_num.attr,
	&driver_attr_end_of_sui_reaction_ptr.attr,
	&driver_attr_sre_pscl.attr,
	&driver_attr_sre_max_block_num.attr,
	&driver_attr_sre_max_instruction_limit.attr,
	&driver_attr_sre_max_index_size.attr,
	&driver_attr_sre_max_offset_ctrl.attr,
	&driver_attr_src_id.attr,
	&driver_attr_liodnr.attr,
	&driver_attr_rev1.attr,
	&driver_attr_rev2.attr,
	&driver_attr_isr.attr,
	&driver_attr_bsc_0.attr,
	&driver_attr_bsc_1.attr,
	&driver_attr_bsc_2.attr,
	&driver_attr_bsc_3.attr,
	&driver_attr_bsc_4.attr,
	&driver_attr_bsc_5.attr,
	&driver_attr_bsc_6.attr,
	&driver_attr_bsc_7.attr,
	&driver_attr_bsc_8.attr,
	&driver_attr_bsc_9.attr,
	&driver_attr_bsc_10.attr,
	&driver_attr_bsc_11.attr,
	&driver_attr_bsc_12.attr,
	&driver_attr_bsc_13.attr,
	&driver_attr_bsc_14.attr,
	&driver_attr_bsc_15.attr,
	&driver_attr_bsc_16.attr,
	&driver_attr_bsc_17.attr,
	&driver_attr_bsc_18.attr,
	&driver_attr_bsc_19.attr,
	&driver_attr_bsc_20.attr,
	&driver_attr_bsc_21.attr,
	&driver_attr_bsc_22.attr,
	&driver_attr_bsc_23.attr,
	&driver_attr_bsc_24.attr,
	&driver_attr_bsc_25.attr,
	&driver_attr_bsc_26.attr,
	&driver_attr_bsc_27.attr,
	&driver_attr_bsc_28.attr,
	&driver_attr_bsc_29.attr,
	&driver_attr_bsc_30.attr,
	&driver_attr_bsc_31.attr,
	&driver_attr_bsc_32.attr,
	&driver_attr_bsc_33.attr,
	&driver_attr_bsc_34.attr,
	&driver_attr_bsc_35.attr,
	&driver_attr_bsc_36.attr,
	&driver_attr_bsc_37.attr,
	&driver_attr_bsc_38.attr,
	&driver_attr_bsc_39.attr,
	&driver_attr_bsc_40.attr,
	&driver_attr_bsc_41.attr,
	&driver_attr_bsc_42.attr,
	&driver_attr_bsc_43.attr,
	&driver_attr_bsc_44.attr,
	&driver_attr_bsc_45.attr,
	&driver_attr_bsc_46.attr,
	&driver_attr_bsc_47.attr,
	&driver_attr_bsc_48.attr,
	&driver_attr_bsc_49.attr,
	&driver_attr_bsc_50.attr,
	&driver_attr_bsc_51.attr,
	&driver_attr_bsc_52.attr,
	&driver_attr_bsc_53.attr,
	&driver_attr_bsc_54.attr,
	&driver_attr_bsc_55.attr,
	&driver_attr_bsc_56.attr,
	&driver_attr_bsc_57.attr,
	&driver_attr_bsc_58.attr,
	&driver_attr_bsc_59.attr,
	&driver_attr_bsc_60.attr,
	&driver_attr_bsc_61.attr,
	&driver_attr_bsc_62.attr,
	&driver_attr_bsc_63.attr,
	NULL
};

static struct attribute *pme_drv_stats_attrs[] = {
	&driver_attr_update_interval.attr,
	&driver_attr_trunci.attr,
	&driver_attr_rbc.attr,
	&driver_attr_tbt0ecc1ec.attr,
	&driver_attr_tbt1ecc1ec.attr,
	&driver_attr_vlt0ecc1ec.attr,
	&driver_attr_vlt1ecc1ec.attr,
	&driver_attr_cmecc1ec.attr,
	&driver_attr_dxcmecc1ec.attr,
	&driver_attr_dxemecc1ec.attr,
	&driver_attr_stnib.attr,
	&driver_attr_stnis.attr,
	&driver_attr_stnth1.attr,
	&driver_attr_stnth2.attr,
	&driver_attr_stnthv.attr,
	&driver_attr_stnths.attr,
	&driver_attr_stnch.attr,
	&driver_attr_stnpm.attr,
	&driver_attr_stns1m.attr,
	&driver_attr_stnpmr.attr,
	&driver_attr_stndsr.attr,
	&driver_attr_stnesr.attr,
	&driver_attr_stns1r.attr,
	&driver_attr_stnob.attr,
	&driver_attr_mia_byc.attr,
	&driver_attr_mia_blc.attr,
	NULL
};

static struct attribute_group pme_drv_attr_grp = {
	.attrs = pme_drv_attrs
};

static struct attribute_group pme_drv_stats_attr_grp = {
	.name  = "stats",
	.attrs = pme_drv_stats_attrs
};

static struct attribute_group *pme_drv_attr_groups[] = {
	&pme_drv_attr_grp,
	&pme_drv_stats_attr_grp,
	NULL,
};

static struct of_device_id of_fsl_pme_ids[] = {
	{ .compatible = "fsl,pme", },
	{}
};

/* Pme interrupt handler */
static irqreturn_t pme_isr(int irq, void *ptr)
{
	static u32 last_isrstate;
	u32 isrstate = pme_in(global_pme, ISR) ^ last_isrstate;

	/* What new ISR state has been raise */
	if (!isrstate)
		return IRQ_NONE;
	if (isrstate & PME_SBE_ERR)
		pr_crit("PME: SBE detected\n");
	if (isrstate & PME_DBE_ERR)
		pr_crit("PME: DBE detected\n");
	if (isrstate & PME_PME_ERR)
		pr_crit("PME: PME serious detected\n");
	/* Clear the ier interrupt bit */
	last_isrstate |= isrstate;
	pme_out(global_pme, IER, ~last_isrstate);
	return IRQ_HANDLED;
}

static int of_fsl_pme_remove(struct of_device *ofdev)
{
	/* Cancel pme accumulator */
	accumulator_update_interval(0);
	cancel_delayed_work_sync(&accumulator_work);
	/* Disable PME..TODO need to wait till it's quiet */
	pme_out(global_pme, FACONF, PME_FACONF_RESET);

	/* Release interrupt */
	free_irq(pme_err_irq, &ofdev->dev);

	/* Unmap controller region */
	iounmap(global_pme);
	return 0;
}

static int __devinit of_fsl_pme_probe(struct of_device *ofdev,
				const struct of_device_id *match)
{
	u32 __iomem *regs;
	struct device *dev;
	struct device_node *nprop;
	u32 clkfreq = DEFAULT_SRFCC * 1000000;
	const u32 *value;
	int srec_aim = 0, srec_esr = 0;
	u32 srecontextsize_code;

	dev = &ofdev->dev;
	nprop = ofdev->node;

	pme_err_irq = of_irq_to_resource(nprop, 0, NULL);
	if (pme_err_irq == NO_IRQ) {
		dev_err(dev, "Can't get %s property '%s'\n", nprop->full_name,
			"interrupts");
		return -ENODEV;
	}

	/* Get configuration properties from device tree */
	/* First, get register page */
	regs = of_iomap(nprop, 0);
	if (regs == NULL) {
		dev_err(dev, "of_iomap() failed\n");
		return -EINVAL;
	}
	/* Global configuration */
	global_pme = pme_create(regs);

	/* Register the pme ISR handler */
	if (request_irq(pme_err_irq, pme_isr, IRQF_SHARED, "pme-err", dev)) {
		dev_err(dev, "request_irq() failed\n");
		return -ENODEV;
	}

#ifdef CONFIG_FSL_PME2_SRE_AIM
	srec_aim = 1;
#endif
#ifdef CONFIG_FSL_PME2_SRE_ESR
	srec_esr = 1;
#endif
	/* Validate some parameters */
	if (!sre_session_ctx_size || !is_power_of_2(sre_session_ctx_size) ||
			(sre_session_ctx_size < 32) ||
			(sre_session_ctx_size > (131072))) {
		dev_err(dev, "invalid sre_session_ctx_size\n");
		iounmap(global_pme);
		return -EINVAL;
	}
	srecontextsize_code = ilog2(sre_session_ctx_size);
	srecontextsize_code -= 4;

	value = of_get_property(nprop, "clock-frequency", NULL);
	if (value)
		clkfreq = *value;

	pme_out(global_pme, SFRCC, clkfreq/1000000);
	BUG_ON(sizeof(dxe_a) != 4);
	pme_out(global_pme, PDSRBAL, (u32)dxe_a);
	pme_out(global_pme, SCBARL, (u32)sre_a);
	/* Maximum allocated index into the PDSR table available to the DXE */
	pme_out(global_pme, DEC1, (dxe_sz/PDSR_TBL_ALIGN)-1);
	/* Maximum allocated index into the PDSR table available to the SRE */
	pme_out(global_pme, SEC2, (dxe_sz/PDSR_TBL_ALIGN)-1);
	/* Maximum allocated 32-byte offset into SRE Context Table.*/
	if (sre_sz)
		pme_out(global_pme, SEC3, (sre_sz/SRE_TBL_ALIGN)-1);
	/* Max test line execution */
	pme_out(global_pme, DEC0, max_test_line_per_pat);
	pme_out(global_pme, DLC,
		(max_pat_eval_per_sui << 16) | max_pat_matches_per_sui);

	/* SREC - SRE Config */
	pme_out(global_pme, SREC,
		/* Number of rules in database */
		(sre_rule_num << 0) |
		/* Simple Report Enabled */
		((srec_esr ? 1 : 0) << 18) |
		/* Context Size per Session */
		(srecontextsize_code << 19) |
		/* Alternate Inclusive Mode */
		((srec_aim ? 1 : 0) << 29));

	pme_out(global_pme, SEC1,
		(CONFIG_FSL_PME2_SRE_MAX_INSTRUCTION_LIMIT << 16) |
		CONFIG_FSL_PME2_SRE_MAX_BLOCK_NUMBER);

	/* Setup Accumulator */
	if (pme_stat_interval)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(pme_stat_interval));

	/* Enable interrupts */
	pme_out(global_pme, IER, PME_ALL_ERR);

	dev_info(dev, "ver: 0x%08x\n", pme_in(global_pme, PM_IP_REV1));
	return 0;
}

static struct of_platform_driver of_fsl_pme_driver = {
	.name = "of-fsl-pme",
	.match_table = of_fsl_pme_ids,
	.probe = of_fsl_pme_probe,
	.driver = {
		.groups = pme_drv_attr_groups,
	},
	.remove      = __devexit_p(of_fsl_pme_remove),
};

static int pme2_ctrl_init(void)
{
	return of_register_platform_driver(&of_fsl_pme_driver);
}

static void pme2_ctrl_exit(void)
{
	of_unregister_platform_driver(&of_fsl_pme_driver);
}

module_init(pme2_ctrl_init);
module_exit(pme2_ctrl_exit);

/************
 * Section 3
 ************
 * These APIs are the only functional hooks into the control driver, besides the
 * sysfs attributes.
 */

int pme2_have_control(void)
{
	return global_pme ? 1 : 0;
}
EXPORT_SYMBOL(pme2_have_control);

int pme2_exclusive_set(struct qman_fq *fq)
{
	if (!pme2_have_control())
		return -ENODEV;
	pme_out(global_pme, EFQC, PME_EFQC(1, qman_fq_fqid(fq)));
	return 0;
}
EXPORT_SYMBOL(pme2_exclusive_set);

int pme2_exclusive_unset(void)
{
	if (!pme2_have_control())
		return -ENODEV;
	pme_out(global_pme, EFQC, PME_EFQC(0, 0));
	return 0;
}
EXPORT_SYMBOL(pme2_exclusive_unset);

int pme_attr_set(enum pme_attr attr, u32 val)
{
	u32 mask;
	u32 attr_val;

	if (!pme2_have_control())
		return -ENODEV;

	/* Check if Buffer size configuration */
	if (attr >= pme_attr_bsc_first && attr <= pme_attr_bsc_last) {
		u32 bsc_pool_id = attr - pme_attr_bsc_first;
		u32 bsc_pool_offset = bsc_pool_id % 8;
		u32 bsc_pool_mask = ~(0xF << ((7-bsc_pool_offset)*4));
		/* range for val 0..0xB */
		if (val > 0xb)
			return -EINVAL;
		/* calculate which sky-blue reg */
		/* 0..7 -> bsc_(0..7), PME_REG_BSC0 */
		/* 8..15 -> bsc_(8..15) PME_REG_BSC1*/
		/* ... */
		/* 56..63 -> bsc_(56..63) PME_REG_BSC7*/
		attr_val = pme_in(global_pme, BSC0 + ((bsc_pool_id/8)*4));
		/* Now mask in the new value */
		attr_val = attr_val & bsc_pool_mask;
		attr_val = attr_val | (val << ((7-bsc_pool_offset)*4));
		pme_out(global_pme,  BSC0 + ((bsc_pool_id/8)*4), attr_val);
		return 0;
	}

	switch (attr) {
	case pme_attr_efqc_int:
		if (val > 4)
			return -EINVAL;
		mask = 0x8FFFFFFF;
		attr_val = pme_in(global_pme, EFQC);
		/* clear efqc_int */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= ~mask;
		val <<= 28;
		val |= attr_val;
		pme_out(global_pme, EFQC, val);
		break;

	case pme_attr_sw_db:
		pme_out(global_pme, SWDB, val);
		break;

	case pme_attr_dmcr:
		pme_out(global_pme, DMCR, val);
		break;

	case pme_attr_smcr:
		pme_out(global_pme, SMCR, val);
		break;

	case pme_attr_famcr:
		pme_out(global_pme, FAMCR, val);
		break;

	case pme_attr_kvlts:
		if (val < 2 || val > 16)
			return -EINVAL;
		/* HW range: 1..15, SW range: 2..16 */
		pme_out(global_pme, KVLTS, --val);
		break;

	case pme_attr_max_chain_length:
		if (val > 0x7FFF)
			val = 0x7FFF;
		pme_out(global_pme, KEC, val);
		break;

	case pme_attr_pattern_range_counter_idx:
		if (val > 0x1FFFF)
			val = 0x1FFFF;
		pme_out(global_pme, DRCIC, val);
		break;

	case pme_attr_pattern_range_counter_mask:
		if (val > 0x1FFFF)
			val = 0x1FFFF;
		pme_out(global_pme, DRCMC, val);
		break;

	case pme_attr_max_allowed_test_line_per_pattern:
		if (val > 0x3FFF)
			val = 0x3FFF;
		pme_out(global_pme, DEC0, val);
		break;

	case pme_attr_max_pattern_matches_per_sui:
		/* mpe, mpm */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0xFFFF0000;
		attr_val = pme_in(global_pme, DLC);
		/* clear mpm */
		attr_val &= mask;
		val &= ~mask;
		val |= attr_val;
		pme_out(global_pme, DLC, val);
		break;

	case pme_attr_max_pattern_evaluations_per_sui:
		/* mpe, mpm */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0x0000FFFF;
		attr_val = pme_in(global_pme, DLC);
		/* clear mpe */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= mask;
		val <<= 16;
		val |= attr_val;
		pme_out(global_pme, DLC, val);
		break;

	case pme_attr_report_length_limit:
		if (val > 0xFFFF)
			val = 0xFFFF;
		pme_out(global_pme, RLL, val);
		break;

	case pme_attr_end_of_simple_sui_report:
		/* bit 13 */
		mask = 0x00040000;
		attr_val = pme_in(global_pme, SREC);
		if (val)
			attr_val |= mask;
		else
			attr_val &= ~mask;
		pme_out(global_pme, SREC, attr_val);
		break;

	case pme_attr_aim:
		/* bit 2 */
		mask = 0x20000000;
		attr_val = pme_in(global_pme, SREC);
		if (val)
			attr_val |= mask;
		else
			attr_val &= ~mask;
		pme_out(global_pme, SREC, attr_val);
		break;

	case pme_attr_end_of_sui_reaction_ptr:
		if (val > 0xFFFFF)
			val = 0xFFFFF;
		pme_out(global_pme, ESRP, val);
		break;

	case pme_attr_sre_pscl:
		pme_out(global_pme, SFRCC, val);
		break;

	case pme_attr_sre_max_block_num:
		/* bits 17..31 */
		if (val > 0x7FFF)
			val = 0x7FFF;
		mask = 0xFFFF8000;
		attr_val = pme_in(global_pme, SEC1);
		/* clear mbn */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= ~mask;
		val |= attr_val;
		pme_out(global_pme, SEC1, val);
		break;

	case pme_attr_sre_max_instruction_limit:
		/* bits 0..15 */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0x0000FFFF;
		attr_val = pme_in(global_pme, SEC1);
		/* clear mil */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= mask;
		val <<= 16;
		val |= attr_val;
		pme_out(global_pme, SEC1, val);
		break;

	case pme_attr_srrv0:
		pme_out(global_pme, SRRV0, val);
		break;
	case pme_attr_srrv1:
		pme_out(global_pme, SRRV1, val);
		break;
	case pme_attr_srrv2:
		pme_out(global_pme, SRRV2, val);
		break;
	case pme_attr_srrv3:
		pme_out(global_pme, SRRV3, val);
		break;
	case pme_attr_srrv4:
		pme_out(global_pme, SRRV4, val);
		break;
	case pme_attr_srrv5:
		pme_out(global_pme, SRRV5, val);
		break;
	case pme_attr_srrv6:
		pme_out(global_pme, SRRV6, val);
		break;
	case pme_attr_srrv7:
		pme_out(global_pme, SRRV7, val);
		break;
	case pme_attr_srrfi:
		pme_out(global_pme, SRRFI, val);
		break;
	case pme_attr_srri:
		pme_out(global_pme, SRRI, val);
		break;
	case pme_attr_srrwc:
		pme_out(global_pme, SRRWC, val);
		break;
	case pme_attr_srrr:
		pme_out(global_pme, SRRR, val);
		break;

	default:
		pr_err("pme: Unknown attr %u\n", attr);
		return -EINVAL;
	};
	return 0;
}
EXPORT_SYMBOL(pme_attr_set);

int pme_attr_get(enum pme_attr attr, u32 *val)
{
	u32 mask;
	u32 attr_val;

	if (!pme2_have_control())
		return -ENODEV;

	/* Check if Buffer size configuration */
	if (attr >= pme_attr_bsc_first && attr <= pme_attr_bsc_last) {
		u32 bsc_pool_id = attr - pme_attr_bsc_first;
		u32 bsc_pool_offset = bsc_pool_id % 8;
		/* calculate which sky-blue reg */
		/* 0..7 -> bsc_(0..7), PME_REG_BSC0 */
		/* 8..15 -> bsc_(8..15) PME_REG_BSC1*/
		/* ... */
		/* 56..63 -> bsc_(56..63) PME_REG_BSC7*/
		attr_val = pme_in(global_pme, BSC0 + ((bsc_pool_id/8)*4));
		attr_val = attr_val >> ((7-bsc_pool_offset)*4);
		attr_val = attr_val & 0x0000000F;
		*val = attr_val;
		return 0;
	}

	switch (attr) {
	case pme_attr_efqc_int:
		mask = 0x8FFFFFFF;
		attr_val = pme_in(global_pme, EFQC);
		attr_val &= ~mask;
		attr_val >>= 28;
		break;

	case pme_attr_sw_db:
		attr_val = pme_in(global_pme, SWDB);
		break;

	case pme_attr_dmcr:
		attr_val = pme_in(global_pme, DMCR);
		break;

	case pme_attr_smcr:
		attr_val = pme_in(global_pme, SMCR);
		break;

	case pme_attr_famcr:
		attr_val = pme_in(global_pme, FAMCR);
		break;

	case pme_attr_kvlts:
		/* bit 28-31 */
		attr_val = pme_in(global_pme, KVLTS);
		attr_val &= 0x0000000F;
		/* HW range: 1..15, SW range: 2..16 */
		attr_val += 1;
		break;

	case pme_attr_max_chain_length:
		/* bit 17-31 */
		attr_val = pme_in(global_pme, KEC);
		attr_val &= 0x00007FFF;
		break;

	case pme_attr_pattern_range_counter_idx:
		/* bit 15-31 */
		attr_val = pme_in(global_pme, DRCIC);
		attr_val &= 0x0001FFFF;
		break;

	case pme_attr_pattern_range_counter_mask:
		/* bit 15-31 */
		attr_val = pme_in(global_pme, DRCMC);
		attr_val &= 0x0001FFFF;
		break;

	case pme_attr_max_allowed_test_line_per_pattern:
		/* bit 18-31 */
		attr_val = pme_in(global_pme, DEC0);
		attr_val &= 0x00003FFF;
		break;

	case pme_attr_max_pdsr_index:
		/* bit 12-31 */
		attr_val = pme_in(global_pme, DEC1);
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_max_pattern_matches_per_sui:
		attr_val = pme_in(global_pme, DLC);
		attr_val &= 0x0000FFFF;
		break;

	case pme_attr_max_pattern_evaluations_per_sui:
		attr_val = pme_in(global_pme, DLC);
		attr_val >>= 16;
		break;

	case pme_attr_report_length_limit:
		attr_val = pme_in(global_pme, RLL);
		/* clear unwanted bits in val*/
		attr_val &= 0x0000FFFF;
		break;

	case pme_attr_end_of_simple_sui_report:
		/* bit 13 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 18;
		/* clear unwanted bits in val*/
		attr_val &= 0x00000001;
		break;

	case pme_attr_aim:
		/* bit 2 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 29;
		/* clear unwanted bits in val*/
		attr_val &= 0x00000001;
		break;

	case pme_attr_sre_context_size:
		/* bits 9..12 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 19;
		/* clear unwanted bits in val*/
		attr_val &= 0x0000000F;
		attr_val += 4;
		attr_val = 1 << attr_val;
		break;

	case pme_attr_sre_rule_num:
		/* bits 24..31 */
		attr_val = pme_in(global_pme, SREC);
		/* clear unwanted bits in val*/
		attr_val &= 0x000003FF;
		break;

	case pme_attr_sre_session_ctx_num: {
			u32 ctx_sz = 0;
			/* = sre_table_size / sre_session_ctx_size */
			attr_val = pme_in(global_pme, SEC3);
			/* clear unwanted bits in val*/
			attr_val &= 0x07FFFFFF;
			attr_val += 1;
			attr_val *= 32;
			ctx_sz = pme_in(global_pme, SREC);
			ctx_sz >>= 19;
			/* clear unwanted bits in val*/
			ctx_sz &= 0x0000000F;
			ctx_sz += 4;
			attr_val /= (1 << ctx_sz);
		}
		break;

	case pme_attr_end_of_sui_reaction_ptr:
		/* bits 12..31 */
		attr_val = pme_in(global_pme, ESRP);
		/* clear unwanted bits in val*/
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_sre_pscl:
		/* bits 22..31 */
		attr_val = pme_in(global_pme, SFRCC);
		/* clear unwanted bits in val*/
		attr_val &= 0x000000FF;
		break;

	case pme_attr_sre_max_block_num:
		/* bits 17..31 */
		attr_val = pme_in(global_pme, SEC1);
		/* clear unwanted bits in val*/
		attr_val &= 0x00007FFF;
		break;

	case pme_attr_sre_max_instruction_limit:
		/* bits 0..15 */
		attr_val = pme_in(global_pme, SEC1);
		attr_val >>= 16;
		break;

	case pme_attr_sre_max_index_size:
		/* bits 12..31 */
		attr_val = pme_in(global_pme, SEC2);
		/* clear unwanted bits in val*/
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_sre_max_offset_ctrl:
		/* bits 5..31 */
		attr_val = pme_in(global_pme, SEC3);
		/* clear unwanted bits in val*/
		attr_val &= 0x07FFFFFF;
		break;

	case pme_attr_src_id:
		/* bits 24..31 */
		attr_val = pme_in(global_pme, SRCIDR);
		/* clear unwanted bits in val*/
		attr_val &= 0x000000FF;
		break;

	case pme_attr_liodnr:
		/* bits 20..31 */
		attr_val = pme_in(global_pme, LIODNR);
		/* clear unwanted bits in val*/
		attr_val &= 0x00000FFF;
		break;

	case pme_attr_rev1:
		/* bits 0..31 */
		attr_val = pme_in(global_pme, PM_IP_REV1);
		break;

	case pme_attr_rev2:
		/* bits 0..31 */
		attr_val = pme_in(global_pme, PM_IP_REV2);
		break;

	case pme_attr_srrr:
		attr_val = pme_in(global_pme, SRRR);
		break;

	case pme_attr_trunci:
		attr_val = pme_in(global_pme, TRUNCI);
		break;

	case pme_attr_rbc:
		attr_val = pme_in(global_pme, RBC);
		break;

	case pme_attr_tbt0ecc1ec:
		attr_val = pme_in(global_pme, TBT0ECC1EC);
		break;

	case pme_attr_tbt1ecc1ec:
		attr_val = pme_in(global_pme, TBT1ECC1EC);
		break;

	case pme_attr_vlt0ecc1ec:
		attr_val = pme_in(global_pme, VLT0ECC1EC);
		break;

	case pme_attr_vlt1ecc1ec:
		attr_val = pme_in(global_pme, VLT1ECC1EC);
		break;

	case pme_attr_cmecc1ec:
		attr_val = pme_in(global_pme, CMECC1EC);
		break;

	case pme_attr_dxcmecc1ec:
		attr_val = pme_in(global_pme, DXCMECC1EC);
		break;

	case pme_attr_dxemecc1ec:
		attr_val = pme_in(global_pme, DXEMECC1EC);
		break;

	case pme_attr_stnib:
		attr_val = pme_in(global_pme, STNIB);
		break;

	case pme_attr_stnis:
		attr_val = pme_in(global_pme, STNIS);
		break;

	case pme_attr_stnth1:
		attr_val = pme_in(global_pme, STNTH1);
		break;

	case pme_attr_stnth2:
		attr_val = pme_in(global_pme, STNTH2);
		break;

	case pme_attr_stnthv:
		attr_val = pme_in(global_pme, STNTHV);
		break;

	case pme_attr_stnths:
		attr_val = pme_in(global_pme, STNTHS);
		break;

	case pme_attr_stnch:
		attr_val = pme_in(global_pme, STNCH);
		break;

	case pme_attr_stnpm:
		attr_val = pme_in(global_pme, STNPM);
		break;

	case pme_attr_stns1m:
		attr_val = pme_in(global_pme, STNS1M);
		break;

	case pme_attr_stnpmr:
		attr_val = pme_in(global_pme, STNPMR);
		break;

	case pme_attr_stndsr:
		attr_val = pme_in(global_pme, STNDSR);
		break;

	case pme_attr_stnesr:
		attr_val = pme_in(global_pme, STNESR);
		break;

	case pme_attr_stns1r:
		attr_val = pme_in(global_pme, STNS1R);
		break;

	case pme_attr_stnob:
		attr_val = pme_in(global_pme, STNOB);
		break;

	case pme_attr_mia_byc:
		attr_val = pme_in(global_pme, MIA_BYC);
		break;

	case pme_attr_mia_blc:
		attr_val = pme_in(global_pme, MIA_BLC);
		break;

	case pme_attr_isr:
		attr_val = pme_in(global_pme, ISR);
		break;

	default:
		pr_err("pme: Unknown attr %u\n", attr);
		return -EINVAL;
	};
	*val = attr_val;
	return 0;
}
EXPORT_SYMBOL(pme_attr_get);

static enum pme_attr stat_list[] = {
	pme_attr_trunci,
	pme_attr_rbc,
	pme_attr_tbt0ecc1ec,
	pme_attr_tbt1ecc1ec,
	pme_attr_vlt0ecc1ec,
	pme_attr_vlt1ecc1ec,
	pme_attr_cmecc1ec,
	pme_attr_dxcmecc1ec,
	pme_attr_dxemecc1ec,
	pme_attr_stnib,
	pme_attr_stnis,
	pme_attr_stnth1,
	pme_attr_stnth2,
	pme_attr_stnthv,
	pme_attr_stnths,
	pme_attr_stnch,
	pme_attr_stnpm,
	pme_attr_stns1m,
	pme_attr_stnpmr,
	pme_attr_stndsr,
	pme_attr_stnesr,
	pme_attr_stns1r,
	pme_attr_stnob,
	pme_attr_mia_byc,
	pme_attr_mia_blc
};

static u64 pme_stats[sizeof(stat_list)/sizeof(enum pme_attr)];
static DEFINE_SPINLOCK(stat_lock);

int pme_stat_get(enum pme_attr *stat, u64 *value, int reset)
{
	int i, ret = 0;
	int value_set = 0;
	u32 val;

	spin_lock_irq(&stat_lock);
	if (stat == NULL || value == NULL) {
		pr_err("pme: Invalid stat request %d\n", *stat);
		ret = -EINVAL;
	} else {
		for (i = 0; i < sizeof(stat_list)/sizeof(enum pme_attr); i++) {
			if (stat_list[i] == *stat) {
				ret = pme_attr_get(stat_list[i], &val);
				/* Do I need to check ret */
				pme_stats[i] += val;
				*value = pme_stats[i];
				value_set = 1;
				if (reset)
					pme_stats[i] = 0;
				break;
			}
		}
		if (!value_set) {
			pr_err("pme: Invalid stat request %d\n", *stat);
			ret = -EINVAL;
		}
	}
	spin_unlock_irq(&stat_lock);
	return ret;
}
EXPORT_SYMBOL(pme_stat_get);

static void accumulator_update_interval(u32 interval)
{
	int schedule = 0;

	spin_lock_irq(&stat_lock);
	if (!pme_stat_interval && interval)
		schedule = 1;
	pme_stat_interval = interval;
	spin_unlock_irq(&stat_lock);
	if (schedule)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(interval));
}

static void accumulator_update(struct work_struct *work)
{
	int i, ret;
	u32 local_interval;
	u32 val;

	spin_lock_irq(&stat_lock);
	local_interval = pme_stat_interval;
	for (i = 0; i < sizeof(stat_list)/sizeof(enum pme_attr); i++) {
		ret = pme_attr_get(stat_list[i], &val);
		pme_stats[i] += val;
	}
	spin_unlock_irq(&stat_lock);
	if (local_interval)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(local_interval));
}

