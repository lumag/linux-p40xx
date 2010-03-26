/*
 * Freescale hypervisor console driver
 *
 * Copyright (C) 2008-2009 Freescale Semiconductor, Inc. All rights reserved.
 * Author: Timur Tabi <timur@freescale.com>
 *
 * This code is based on drivers/char/hvc_beat.c:
 * (C) Copyright 2006 TOSHIBA CORPORATION
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/console.h>
#include <linux/irq.h>
#include <asm/fsl_hcalls.h>
#include <asm/udbg.h>
#include <linux/of.h>

#include "hvc_console.h"

/**
 * hvc_fsl_get_chars - fetch characters from the byte stream
 *
 * @param handle - the byte channel handle
 * @param buffer - pointer to a buffer of at least 'size' bytes in length
 * @param size - the size of the buffer, in bytes
 *
 * Returns the number of characters received
 */
static int hvc_fsl_get_chars(uint32_t handle, char *buffer, int size)
{
	unsigned int count = min(16, size);
	unsigned int rx_count;
	unsigned int tx_count;
	int32_t ret;

	ret = fh_byte_channel_poll(handle, &rx_count, &tx_count);
	if (ret || !rx_count)
		return 0;

	ret = fh_byte_channel_receive(handle, &count, buffer);

	return ret ? 0 : count;
}

/**
 * hvc_fsl_put_chars - send characters to the hypervisor
 *
 * @handle - the byte channel handle
 * @buffer - pointer to a buffer (must be at least 16 bytes long)
 * @count - the number of bytes to send
 */
static int hvc_fsl_put_chars(uint32_t handle, const char *buffer, int count)
{
	int rest;
	unsigned int nlen;
	int32_t ret;

	for (rest = count; rest > 0; rest -= nlen) {
		nlen = min(16, rest);
		ret = fh_byte_channel_send(handle, nlen, buffer);
		if (ret)
			break;
		buffer += nlen;
	}

	return count - rest;
}
/**
 * hvc_fsl_get_put_ops - Hypervisor console operations
 */
static struct hv_ops hvc_fsl_get_put_ops = {
	.get_chars = hvc_fsl_get_chars,
	.put_chars = hvc_fsl_put_chars,
};

/**
 *  Find the byte channel node to use for the console and TTY
 */
static struct device_node *find_byte_channel(void)
{
	static struct device_node *np;
	const char *sprop;

	if (np)
		return np;

	np = of_find_node_by_name(NULL, "aliases");
	if (!np) {
		pr_debug("hvc-fsl: no 'aliases' node\n");
		return NULL;
	}

	sprop = of_get_property(np, "stdout", NULL);
	if (!sprop) {
		pr_debug("hvc-fsl: no 'stdout' property\n");
		return NULL;
	}

	/*
	 * We don't care what the aliased node is actually called.  We only
	 * care if it's compatible with "fsl,hv-byte-channel", because that
	 * indicates that it's a byte channel node.
	 */
	np = of_find_node_by_path(sprop);
	if (!np) {
		pr_warning("hvc-fsl: stdout node '%s' does not exist\n", sprop);
		return NULL;
	}

	if (!of_device_is_compatible(np, "fsl,hv-byte-channel-handle")) {
		pr_debug("hvc-fsl: node '%s' is not a byte channel\n", sprop);
		return NULL;
	}

	return np;
}

/**
 * has_fsl_hypervisor - return TRUE if we're running under FSL hypervisor
 *
 * This function checks to see if we're running under the Freescale
 * hypervisor, and returns zero if we're not, or non-zero if we are.
 *
 * First, it checks if MSR[GS]==1, which means we're running under some
 * hypervisor.  Then it checks if there is a hypervisor node in the device
 * tree.  Currently, that means there needs to be a node in the root called
 * "hypervisor" and which has a property named "fsl,hv-version".
 */
static int has_fsl_hypervisor(void)
{
	struct device_node *node;
	int ret;

	if (!(mfmsr() & MSR_GS))
		return 0;

	node = of_find_node_by_path("/hypervisor");
	if (!node)
		return 0;

	ret = of_find_property(node, "fsl,hv-version", NULL) != NULL;

	of_node_put(node);

	return ret;
}

/**
 * hvc_fsl_console_init - initialize the console interface
 *
 * This reads the device tree to determine the byte channel number that
 * corresponds to stdout.  An error is returned if that information can't be
 * found.
 */
static int __init hvc_fsl_console_init(void)
{
	struct device_node *np;
	const u32 *iprop;
	int ret;

	if (!has_fsl_hypervisor()) {
		pr_debug("hvc-fsl: no hypervisor found\n");
		return -ENODEV;
	}

	np = find_byte_channel();
	if (!np) {
		pr_debug("hvc-fsl: no byte-channel node\n");
		return -ENODEV;
	}

	/* The 'reg' property contains the byte channel handle */
	iprop = of_get_property(np, "reg", NULL);
	if (!iprop) {
		pr_err("hvc-fsl: byte-channel node missing 'reg' property\n");
		return -ENODEV;
	}

	ret = add_preferred_console("hvc", 0, NULL);
	if (ret) {
		pr_err("hvc-fsl: could not enable HVC\n");
		return ret;
	}

	ret = hvc_instantiate(*iprop, 0, &hvc_fsl_get_put_ops);
	if (ret) {
		pr_err("hvc-fsl: could not register driver\n");
		return ret;
	}

	pr_info("Freescale hypervisor console driver\n");
	pr_info("hvc-fsl: using byte channel handle %u\n", *iprop);

	return 0;
}

static struct hvc_struct *hvc_fsl_tty_dev;

/**
 * hvc_fsl_init - initialize the TTY interface
 */
static int __init hvc_fsl_init(void)
{
	struct hvc_struct *hp;
	struct device_node *np;
	const u32 *iprop;
	int rx_irq;

	pr_info("Freescale hypervisor TTY driver\n");

	if (!has_fsl_hypervisor()) {
		pr_info("hvc-fsl: no hypervisor found\n");
		return -ENODEV;
	}

	np = find_byte_channel();
	if (!np) {
		pr_debug("hvc-fsl: no byte-channel node\n");
		return -ENODEV;
	}

	iprop = of_get_property(np, "reg", NULL);
	if (!iprop) {
		pr_err("hvc-fsl: byte-channel node missing 'reg' property\n");
		return -ENODEV;
	}

	rx_irq = irq_of_parse_and_map(np, 0);

	pr_debug("hvc-fsl: handle=%u irq=%u\n", *iprop, rx_irq);

	hp = hvc_alloc(*iprop, rx_irq, &hvc_fsl_get_put_ops, 16);
	if (IS_ERR(hp)) {
		pr_err("hvc-fsl: could not register driver\n");
		return PTR_ERR(hp);
	}

	hvc_fsl_tty_dev = hp;

	pr_info("hvc-fsl: using byte channel handle %u\n", *iprop);

	return 0;
}

static void __exit hvc_fsl_exit(void)
{
	if (hvc_fsl_tty_dev)
		hvc_remove(hvc_fsl_tty_dev);

	hvc_fsl_tty_dev = NULL;
}

/**
 * The UDBG byte channel handle.  This needs to be a global variable because
 * the udbg putc() callback function does not take any parameters.
 */
static unsigned int udbg_handle;

/**
 * byte_channel_spin_send - send bytes to a byte channel, wait if necessary
 *
 * This function sends an array of bytes to a byte channel, and it waits and
 * retries if the byte channel is full.  It returns if all data has been
 * sent, or if some error has occurred.
 *
 * 'length' must be less than or equal to 16.  No parameter validation is
 * performed.
 */
static void byte_channel_spin_send(unsigned int handle, const void *data,
				   unsigned int length)
{
	int ret;

	do {
		ret = fh_byte_channel_send(handle, length, data);
		if (!ret)
			break;
	} while (ret == EAGAIN);
}

/**
 * fsl_udbg_putc - udbg put character callback
 *
 * The udbg subsystem calls this function to display a single character.
 * We convert CR to a CR/LF.
 */
static void fsl_udbg_putc(char c)
{
	if (c == '\n')
		byte_channel_spin_send(udbg_handle, "\n\r", 2);
	else
		byte_channel_spin_send(udbg_handle, &c, 1);
}

/**
 * udbg_init_hvc_fsl - early console initialization
 *
 * PowerPC kernels support an early printk console, also known as udbg.
 * This function must be called via the ppc_md.init_early function pointer.
 * At this point, the device tree has been unflattened, so we can obtain the
 * byte channel handle for stdout.
 *
 * We only support displaying of characters (putc).  We do not support
 * keyboard input.
 */
void __init udbg_init_hvc_fsl(void)
{
	struct device_node *np;
	const u32 *iprop;

	if (!has_fsl_hypervisor())
		return;

	np = find_byte_channel();
	if (!np)
		return;

	iprop = of_get_property(np, "reg", NULL);
	if (!iprop)
		return;

	udbg_handle = *iprop;
	udbg_putc = fsl_udbg_putc;
	register_early_udbg_console();

	udbg_printf("Freescale hypervisor early console driver\n");
	udbg_printf("hvc-fsl: using byte channel handle %u\n", udbg_handle);
}

module_init(hvc_fsl_init);
module_exit(hvc_fsl_exit);

console_initcall(hvc_fsl_console_init);
