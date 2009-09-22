/*
 * Freescale hypervisor call interface
 *
 * Copyright (C) 2008-2009 Freescale Semiconductor, Inc.
 * Author: Timur Tabi <timur@freescale.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

/* A "hypercall" is an "sc 1" instruction.  This header file file provides C
 * wrapper functions for the Freescale hypervisor interface.  It is inteded
 * for use by Linux device drivers and other operating systems.
 *
 * The hypercalls are implemented as inline assembly, rather than assembly
 * language functions in a .S file, for optimization.  It allows
 * the caller to issue the hypercall instruction directly, improving both
 * performance and memory footprint.
 */

#ifndef _FREESCALE_HCALLS_H
#define _FREESCALE_HCALLS_H

#include <linux/types.h>
#include <linux/errno.h>
#include <asm/byteorder.h>

#define FH_API_VERSION 1

#define FH_ERR_GET_INFO                 2
#define FH_PARTITION_GET_DTPROP         3
#define FH_PARTITION_SET_DTPROP         4
#define FH_PARTITION_RESTART            5
#define FH_PARTITION_GET_STATUS         6
#define FH_PARTITION_START              7
#define FH_PARTITION_STOP               8
#define FH_PARTITION_MEMCPY             9
#define FH_VMPIC_SET_INT_CONFIG         10
#define FH_VMPIC_GET_INT_CONFIG         11
#define FH_DMA_ENABLE                   12
#define FH_DMA_DISABLE                  13
#define FH_VMPIC_SET_MASK               14
#define FH_VMPIC_GET_MASK               15
#define FH_VMPIC_GET_ACTIVITY           16
#define FH_VMPIC_EOI                    17
#define FH_BYTE_CHANNEL_SEND            18
#define FH_BYTE_CHANNEL_RECEIVE         19
#define FH_BYTE_CHANNEL_POLL            20
#define FH_VMPIC_IACK                   21
#define FH_SEND_NMI                     22
#define FH_VMPIC_GET_MSIR               23
#define FH_SYSTEM_RESET                 24
#define FH_PARTITION_SEND_DBELL         32

/* Extended return codes */

#define FH_ERR_INTERNAL         1024    /* An internal error occured */
#define FH_ERR_CONFIG           1025    /* A configuration error was detected */
#define FH_ERR_INVALID_STATE    1026    /* The object is in an invalid state */
#define FH_ERR_UNIMPLEMENTED    1027    /* Unimplemented hypercall */
#define FH_ERR_BUFFER_OVERFLOW  1028    /* Caller-supplied buffer too small */
#define FH_ERR_TOO_LARGE        1029    /* Argument is too large */

/*
 * Hypercall register clobber list
 *
 * These macros are used to define the list of clobbered registers during a
 * hypercall.  Technically, registers r0 and r3-r12 are always clobbered,
 * but the gcc inline assembly syntax does not allow us to specify registers
 * on the clobber list that are also on the input/output list.  Therefore,
 * the lists of clobbered registers depends on the number of register
 * parmeters ("+r" and "=r") passed to the hypercall.
 *
 * Each assembly block should use one of the HCALL_CLOBBERSx macros.  As a
 * general rule, 'x' is the number of parameters passed to the assembly
 * block *except* for r11.
 *
 * If you're not sure, just use the smallest value of 'x' that does not
 * generate a compilation error.  Because these are static inline functions,
 * the compiler will only check the clobber list for a function if you
 * compile code that calls that function.
 *
 * r3 and r11 are not included in any clobbers list because they are always
 * listed as output registers.
 *
 * XER, CTR, and LR are currently listed as clobbers because it's uncertain
 * whether they will be clobbered.
 *
 * Note that r11 can be used as an output parameter.
*/

/* List of common clobbered registers.  Do not use this macro. */
#define HCALL_CLOBBERS "r0", "r12", "xer", "ctr", "lr", "cc"

#define HCALL_CLOBBERS8 HCALL_CLOBBERS
#define HCALL_CLOBBERS7 HCALL_CLOBBERS8, "r10"
#define HCALL_CLOBBERS6 HCALL_CLOBBERS7, "r9"
#define HCALL_CLOBBERS5 HCALL_CLOBBERS6, "r8"
#define HCALL_CLOBBERS4 HCALL_CLOBBERS5, "r7"
#define HCALL_CLOBBERS3 HCALL_CLOBBERS4, "r6"
#define HCALL_CLOBBERS2 HCALL_CLOBBERS3, "r5"
#define HCALL_CLOBBERS1 HCALL_CLOBBERS2, "r4"

/*
 * We use "uintptr_t" to define a register because it's guaranteed to be a
 * 32-bit integer on a 32-bit platform, and a 64-bit integer on a 64-bit
 * platform.
 *
 * All registers are either input/output or output only.  Registers that are
 * initialized before making the hypercall are input/output.  All
 * input/output registers are represented with "+r".  Output-only registers
 * are represented with "=r".  Do not specify any unused registers.  The
 * clobber list will tell the compiler that the hypercall modifies those
 * registers, which is good enough.
 */

/*
 * The hcall calls below are in numerical order with respect
 * to the hcall token.
 */

/**
 * fh_send_nmi - send NMI to virtual cpu(s).
 * @vcpu_mask: send NMI to virtual cpu(s) specified by this mask.
 *
 * Returns 0 for success, or EINVAL for invalid vcpu_mask.
 */
static inline unsigned int fh_send_nmi(unsigned int vcpu_mask)
{
	register uintptr_t r11 __asm__("r11") = FH_SEND_NMI;
	register uintptr_t r3 __asm__("r3") = vcpu_mask;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/* Arbitrary limits to avoid excessive memory allocation in hypervisor */
#define FH_DTPROP_MAX_PATHLEN 4096
#define FH_DTPROP_MAX_PROPLEN 32768

/**
 * fh_partiton_get_dtprop - get a property from a guest device tree.
 * @handle: handle of partition whose device tree is to be accessed
 * @dtpath_addr: physical address of device tree path to access
 * @propname_addr: physical address of name of property
 * @propvalue_addr: physical address of property value buffer
 * @propvalue_len: length of buffer on entry, length of property on return
 *
 * Returns zero on success, non-zero on error.
 */
static inline unsigned int fh_partition_get_dtprop(int handle,
                                                   uint64_t dtpath_addr,
                                                   uint64_t propname_addr,
                                                   uint64_t propvalue_addr,
                                                   uint32_t *propvalue_len)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_GET_DTPROP;
	register uintptr_t r3 __asm__("r3") = handle;

	register uintptr_t r4 __asm__("r4") = dtpath_addr >> 32;
	register uintptr_t r5 __asm__("r5") = (uint32_t)dtpath_addr;

	register uintptr_t r6 __asm__("r6") = propname_addr >> 32;
	register uintptr_t r7 __asm__("r7") = (uint32_t)propname_addr;

	register uintptr_t r8 __asm__("r8") = propvalue_addr >> 32;
	register uintptr_t r9 __asm__("r9") = (uint32_t)propvalue_addr;

	register uintptr_t r10 __asm__("r10") = *propvalue_len;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11),
		  "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7),
		  "+r" (r8), "+r" (r9), "+r" (r10)
		: : HCALL_CLOBBERS8
	);

	*propvalue_len = r4;
	return r3;
}

/**
 * Set a property in a guest device tree.
 * @handle: handle of partition whose device tree is to be accessed
 * @dtpath_addr: physical address of device tree path to access
 * @propname_addr: physical address of name of property
 * @propvalue_addr: physical address of property value
 * @propvalue_len: length of property
 *
 * Returns zero on success, non-zero on error.
 */
static inline unsigned int fh_partition_set_dtprop(int handle,
                                                   uint64_t dtpath_addr,
                                                   uint64_t propname_addr,
                                                   uint64_t propvalue_addr,
                                                   uint32_t propvalue_len)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_SET_DTPROP;
	register uintptr_t r3 __asm__("r3") = handle;

	register uintptr_t r4 __asm__("r4") = dtpath_addr >> 32;
	register uintptr_t r5 __asm__("r5") = (uint32_t)dtpath_addr;

	register uintptr_t r6 __asm__("r6") = propname_addr >> 32;
	register uintptr_t r7 __asm__("r7") = (uint32_t)propname_addr;

	register uintptr_t r8 __asm__("r8") = propvalue_addr >> 32;
	register uintptr_t r9 __asm__("r9") = (uint32_t)propvalue_addr;

	register uintptr_t r10 __asm__("r10") = propvalue_len;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11),
		  "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7),
		  "+r" (r8), "+r" (r9), "+r" (r10)
		: : HCALL_CLOBBERS8
	);

	return r3;
}


/**
 * fh_partition_restart - reboot the current partition
 * @partition: partition ID
 *
 * Returns an error code if reboot failed.  Does not return if it succeeds.
 */
static inline unsigned int fh_partition_restart(unsigned int partition)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_RESTART;
	register uintptr_t r3 __asm__("r3") = partition;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_partition_get_status - gets the status of a partition
 * @partition: partition ID
 * @status: returned status code
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_partition_get_status(unsigned int partition,
	unsigned int *status)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_GET_STATUS;
	register uintptr_t r3 __asm__("r3") = partition;
	register uintptr_t r4 __asm__("r4");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4)
		: : HCALL_CLOBBERS2
	);

	*status = r4;

	return r3;
}

/**
 * fh_partition_start - boots and starts execution of the specified partition
 * @partition: partition ID
 * @entry_point: guest physical address to start execution
 *
 * The hypervisor creates a 1-to-1 virtual/physical IMA mapping, so at boot
 * time, guest physical address are the same as guest virtual addresses.
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_partition_start(unsigned int partition,
	uint32_t entry_point)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_START;
	register uintptr_t r3 __asm__("r3") = partition;
	register uintptr_t r4 __asm__("r4") = entry_point;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "+r" (r4)
		: : HCALL_CLOBBERS2
	);

	return r3;
}

/**
 * fh_partition_stop - stops another partition
 * @partition: partition ID
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_partition_stop(unsigned int partition)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_STOP;
	register uintptr_t r3 __asm__("r3") = partition;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * struct fh_sg_list: definition of the fh_partition_memcpy S/G list
 * @source: guest physical address to copy from
 * @target: guest physical address to copy to
 * @size: number of bytes to copy
 * @reserved: reserved, must be zero
 *
 * The scatter/gather list for fh_partition_memcpy() is an array of these
 * structures.  The array must be guest physically contiguous.
 *
 * This structure must be aligned on 32-byte boundary, so that no single
 * strucuture can span two pages.
 */
struct fh_sg_list {
	uint64_t source;
	uint64_t target;
	uint64_t size;
	uint64_t reserved;
} __attribute__ ((aligned(32)));

/**
 * fh_partition_memcpy - copies data from one guest to another
 * @source: the ID of the partition to copy from
 * @target: the ID of the partition to copy to
 * @sg_list: guest physical address of an array of &fh_sg_list structures
 * @count: the number of entries in @sg_list
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_partition_memcpy(unsigned int source,
	unsigned int target, phys_addr_t sg_list, unsigned int count)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_MEMCPY;
	register uintptr_t r3 __asm__("r3") = source;
	register uintptr_t r4 __asm__("r4") = target;
	register uintptr_t r5 __asm__("r5") = (uint32_t) sg_list;
#ifdef CONFIG_PHYS_64BIT
	register uintptr_t r6 __asm__("r6") = sg_list >> 32;
#else
	register uintptr_t r6 __asm__("r6") = 0;
#endif
	register uintptr_t r7 __asm__("r7") = count;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11),
		  "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7)
		: : HCALL_CLOBBERS5
	);

	return r3;
}

/**
 * fh_vmpic_set_int_config - configure the specified interrupt
 * @interrupt: the interrupt number
 * @config: configuration for this interrupt
 * @priority: interrupt priority
 * @destination: destination CPU ID mask
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_set_int_config(unsigned int interrupt,
	uint32_t config, unsigned int priority, uint32_t destination)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_SET_INT_CONFIG;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4") = config;
	register uintptr_t r5 __asm__("r5") = priority;
	register uintptr_t r6 __asm__("r6") = destination;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "+r" (r4), "+r" (r5), "+r" (r6)
		: : HCALL_CLOBBERS4
	);

	return r3;
}

/**
 * fh_vmpic_get_int_config - return the config of the specified interrupt
 * @interrupt: the interrupt number
 * @config: returned configuration for this interrupt
 * @priority: returned interrupt priority
 * @destination: returned destination CPU ID mask
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_get_int_config(unsigned int interrupt,
	uint32_t *config, unsigned int *priority, uint32_t *destination)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_GET_INT_CONFIG;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4");
	register uintptr_t r5 __asm__("r5");
	register uintptr_t r6 __asm__("r6");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4), "=r" (r5), "=r" (r6)
		: : HCALL_CLOBBERS4
	);

	*config = r4;
	*priority = r5;
	*destination = r6;

	return r3;
}

/**
 * fh_dma_enable - enable DMA for the specified device
 * @liodn: the LIODN of the I/O device for which to enable DMA
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_dma_enable(unsigned int liodn)
{
	register uintptr_t r11 __asm__("r11") = FH_DMA_ENABLE;
	register uintptr_t r3 __asm__("r3") = liodn;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_dma_disable - disable DMA for the specified device
 * @liodn: the LIODN of the I/O device for which to disable DMA
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_dma_disable(unsigned int liodn)
{
	register uintptr_t r11 __asm__("r11") = FH_DMA_DISABLE;
	register uintptr_t r3 __asm__("r3") = liodn;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_vmpic_set_mask - sets the mask for the specified interrupt source
 * @interrupt: the interrupt number
 * @mask: 0=enable interrupts, 1=disable interrupts
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_set_mask(unsigned int interrupt,
	unsigned int mask)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_SET_MASK;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4") = mask;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "+r" (r4)
		: : HCALL_CLOBBERS2
	);

	return r3;
}

/**
 * fh_vmpic_get_mask - returns the mask for the specified interrupt source
 * @interrupt: the interrupt number
 * @mask: returned mask for this interrupt (0=enabled, 1=disabled)
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_get_mask(unsigned int interrupt,
	unsigned int *mask)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_GET_MASK;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4)
		: : HCALL_CLOBBERS2
	);

	*mask = r4;

	return r3;
}

/**
 * fh_vmpic_get_activity - returns the activity status of an interrupt source
 * @interrupt: the interrupt number
 * @activity: returned activity status.
 *
 * The activity status is a value that indicates whether an interrupt has
 * been requested (i.e. is in service).
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_get_activity(unsigned int interrupt,
	unsigned int *activity)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_GET_ACTIVITY;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4)
		: : HCALL_CLOBBERS2
	);

	*activity = r4;

	return r3;
}

/**
 * fh_vmpic_eoi - signal the end of interrupt processing
 * @interrupt: the interrupt number
 *
 * This function signals the end of processing for the the specified
 * interrupt, which must be the interrupt currently in service. By
 * definition, this is also the highest-priority interrupt.
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_eoi(unsigned int interrupt)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_EOI;
	register uintptr_t r3 __asm__("r3") = interrupt;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_vmpic_get_msir - returns the MPIC-MSI register value
 * @interrupt: the interrupt number
 * @msir_val: returned MPIC-MSI register value
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_get_msir(unsigned int interrupt,
	unsigned int *msir_val)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_GET_MSIR;
	register uintptr_t r3 __asm__("r3") = interrupt;
	register uintptr_t r4 __asm__("r4");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4)
		: : HCALL_CLOBBERS2
	);

	*msir_val = r4;

	return r3;
}

/**
 * fh_byte_channel_send - send characters to a byte stream
 * @handle: byte stream handle
 * @count: number of characters to send
 * @buffer: pointer to a 16-byte buffer
 *
 * @buffer must be at least 16 bytes long, because all 16 bytes will be
 * read from memory into registers, even if count < 16.
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_byte_channel_send(unsigned int handle,
	unsigned int count, const char buffer[16])
{
	register uintptr_t r11 __asm__("r11") = FH_BYTE_CHANNEL_SEND;

	const uint32_t *p = (const uint32_t *) buffer;
	register uintptr_t r3 __asm__("r3") = handle;
	register uintptr_t r4 __asm__("r4") = count;
	register uintptr_t r5 __asm__("r5") = be32_to_cpu(p[0]);
	register uintptr_t r6 __asm__("r6") = be32_to_cpu(p[1]);
	register uintptr_t r7 __asm__("r7") = be32_to_cpu(p[2]);
	register uintptr_t r8 __asm__("r8") = be32_to_cpu(p[3]);

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3),
		  "+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7), "+r" (r8)
		: : HCALL_CLOBBERS6
	);

	return r3;
}

/**
 * fh_byte_channel_receive - fetch characters from a byte channel
 * @handle: byte channel handle
 * @count: (input) max num of chars to receive, (output) num chars received
 * @buffer: pointer to a 16-byte buffer
 *
 * The size of @buffer must be at least 16 bytes, even if you request fewer
 * than 16 characters, because we always write 16 bytes to @buffer.  This is
 * for performance reasons.
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_byte_channel_receive(unsigned int handle,
	unsigned int *count, char buffer[16])
{
	register uintptr_t r11 __asm__("r11") = FH_BYTE_CHANNEL_RECEIVE;
	register uintptr_t r3 __asm__("r3") = handle;
	register uintptr_t r4 __asm__("r4") = *count;
	register uintptr_t r5 __asm__("r5");
	register uintptr_t r6 __asm__("r6");
	register uintptr_t r7 __asm__("r7");
	register uintptr_t r8 __asm__("r8");

	uint32_t *p = (uint32_t *) buffer;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "+r" (r4),
		  "=r" (r5), "=r" (r6), "=r" (r7), "=r" (r8)
		: : HCALL_CLOBBERS6
	);

	*count = r4;
	p[0] = cpu_to_be32(r5);
	p[1] = cpu_to_be32(r6);
	p[2] = cpu_to_be32(r7);
	p[3] = cpu_to_be32(r8);

	return r3;
}

/**
 * fh_byte_channel_poll - returns the status of the byte channel buffers
 * @handle: byte channel handle
 * @rx_count: returned count of bytes in receive queue
 * @tx_count: returned count of bytes in transmit queue
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_byte_channel_poll(unsigned int handle,
	unsigned int *rx_count,	unsigned int *tx_count)
{
	register uintptr_t r11 __asm__("r11") = FH_BYTE_CHANNEL_POLL;
	register uintptr_t r3 __asm__("r3") = handle;
	register uintptr_t r4 __asm__("r4");
	register uintptr_t r5 __asm__("r5");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4), "=r" (r5)
		: : HCALL_CLOBBERS3
	);

	*rx_count = r4;
	*tx_count = r5;

	return r3;
}

/**
 * fh_vmpic_iack - acknowledge an interrupt
 * @vector: returned interrupt vector
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_vmpic_iack(unsigned int *vector)
{
	register uintptr_t r11 __asm__("r11") = FH_VMPIC_IACK;
	register uintptr_t r3 __asm__("r3");
	register uintptr_t r4 __asm__("r4");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "=r" (r3), "=r" (r4)
		: : HCALL_CLOBBERS2
	);

	*vector = r4;

	return r3;
}

/**
 * fh_system_reset - reset the system
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_system_reset(void)
{
	register uintptr_t r11 __asm__("r11") = FH_SYSTEM_RESET;
	register uintptr_t r3 __asm__("r3");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "=r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_partition_send_dbell - send a doorbell to another partition
 * @handle: doorbell send handle
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_partition_send_dbell(unsigned int handle)
{
	register uintptr_t r11 __asm__("r11") = FH_PARTITION_SEND_DBELL;
	register uintptr_t r3 __asm__("r3") = handle;

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3)
		: : HCALL_CLOBBERS1
	);

	return r3;
}

/**
 * fh_err_get_info - get platform error information
 * @queue id:
 * 0 for guest error event queue
 * 1 for global error event queue
 *
 * @pointer to store the platform error data:
 * platform error data is returned in registers r4 - r11
 *
 * Returns 0 for success, or an error code.
 */
static inline unsigned int fh_err_get_info(int queue, uint32_t *error_info)
{
	register uintptr_t r11 __asm__("r11") = FH_ERR_GET_INFO;
	register uintptr_t r3 __asm__("r3") = queue;
	register uintptr_t r4 __asm__("r4");
	register uintptr_t r5 __asm__("r5");
	register uintptr_t r6 __asm__("r6");
	register uintptr_t r7 __asm__("r7");
	register uintptr_t r8 __asm__("r8");
	register uintptr_t r9 __asm__("r9");
	register uintptr_t r10 __asm__("r10");

	__asm__ __volatile__ ("sc 1"
		: "+r" (r11), "+r" (r3), "=r" (r4), "=r" (r5), "=r" (r6), "=r" (r7), "=r" (r8),
			"=r" (r9), "=r" (r10)
		: : HCALL_CLOBBERS8
	);

	error_info[0] = r4;
	error_info[1] = r5;
	error_info[2] = r6;
	error_info[3] = r7;
	error_info[4] = r8;
	error_info[5] = r9;
	error_info[6] = r10;
	error_info[7] = r11;

	return r3;
}
#endif
