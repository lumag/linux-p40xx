/* Freescale hypervisor byte channel character driver
 *
 * Copyright (C) 2009 Freescale Semiconductor, Inc.
 * Author: Timur Tabi <timur@freescale.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 * This driver creates /dev entries for each Freescale hypervisor byte
 * channel, thereby allowing applications to communicate with byte channels
 * via the standard read/write/poll API.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <asm/fsl_hcalls.h>
#include <linux/of.h>
#include <linux/cdev.h>

/* Per-byte channel private data */
struct fsl_bc_data {
	struct device *dev;
	dev_t dev_id;
	uint32_t handle;
	unsigned int rx_irq;
	unsigned int tx_irq;
	wait_queue_head_t rx_wait;
	wait_queue_head_t tx_wait;
	int rx_ready;
	int tx_ready;
};

/**
 * fsl_bc_rx_isr - byte channel receive interupt handler
 *
 * This ISR is called whenever data is available on a byte channel.
 */
static irqreturn_t fsl_bc_rx_isr(int irq, void *data)
{
	struct fsl_bc_data *bc = data;

	bc->rx_ready = 1;
	wake_up_interruptible(&bc->rx_wait);

	return IRQ_HANDLED;
}

/**
 * fsl_bc_tx_isr - byte channel transmit interupt handler
 *
 * This ISR is called whenever space is available on a byte channel.
 */
static irqreturn_t fsl_bc_tx_isr(int irq, void *data)
{
	struct fsl_bc_data *bc = data;

	bc->tx_ready = 1;
	wake_up_interruptible(&bc->tx_wait);

	return IRQ_HANDLED;
}

/**
 * fsl_bc_poll - query the byte channel to see if data is available
 *
 * Returns a bitmask indicating whether a read will block
 */
static unsigned int fsl_bc_poll(struct file *filp, struct poll_table_struct *p)
{
	struct fsl_bc_data *bc = filp->private_data;
	unsigned int rx_count, tx_count;
	unsigned int mask = 0;
	int ret;

	ret = fh_byte_channel_poll(bc->handle, &rx_count, &tx_count);
	if (ret)
		return POLLERR;

	poll_wait(filp, &bc->rx_wait, p);
	poll_wait(filp, &bc->tx_wait, p);

	if (rx_count)
		mask |= POLLIN | POLLRDNORM;
	if (tx_count)
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

/**
 * fsl_bc_read - read from a byte channel into the user's buffer
 *
 * Returns the total number of bytes read, or error
 */
static ssize_t fsl_bc_read(struct file *filp, char __user *buf, size_t len,
			   loff_t *off)
{
	struct fsl_bc_data *bc = filp->private_data;
	unsigned int count;
	unsigned int total = 0;
	char buffer[16];
	int ret;

	/* Make sure we stop when the user buffer is full. */
	while (len) {
		/* Don't ask for more than we can receive */
		count = min(len, sizeof(buffer));

		/* Reset the RX status here so that we don't need a spinlock
		 * around the hyerpcall.  It won't matter if the ISR is called
		 * before the receive() returns, because 'count' will be
		 * non-zero, so we won't test rx_ready.
		 */
		bc->rx_ready = 0;

		/* Non-blocking */
		ret = fh_byte_channel_receive(bc->handle, &count, buffer);
		if (ret)
			return -EIO;

		/* If the byte channel is empty, then either we're done or we
		 * need to block.
		 */
		if (!count) {
			if (total)
				/* We did read some chars, so we're done. */
				return total;

			/* If the application specified O_NONBLOCK, then we
			 * return the appropriate error code.
			 */
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Wait until some data is available */
			if (wait_event_interruptible(bc->rx_wait, bc->rx_ready))
				return -ERESTARTSYS;

			/* Data is available, so loop around and read it */
			continue;
		}

		copy_to_user(buf, buffer, count);

		buf += count;
		len -= count;
		total += count;
	}

	return total;
}

/**
 * fsl_bc_write - write to a byte channel from the user's buffer
 *
 * Returns the total number of bytes written, or error
 */
static ssize_t fsl_bc_write(struct file *filp, const char __user *buf,
			    size_t len, loff_t *off)
{
	struct fsl_bc_data *bc = filp->private_data;
	unsigned int count;
	unsigned int total = 0;
	char buffer[16];
	int ret;

	while (len) {
		count = min(len, sizeof(buffer));
		copy_from_user(buffer, buf, count);

		bc->tx_ready = 0;

		ret = fh_byte_channel_send(bc->handle, count, buffer);
		if (ret) {
			if (total)
				/* We did write some chars, so we're done. */
				return total;

			/* If the application specified O_NONBLOCK, then we
			 * return the appropriate error code.
			 */
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Wait until some data is available */
			if (wait_event_interruptible(bc->tx_wait, bc->tx_ready))
				return -ERESTARTSYS;

			continue;
		}

		buf += count;
		len -= count;
		total += count;
	}

	return total;
}

/* Array of byte channel objects */
static struct fsl_bc_data *bcs;

/* Number of elements in bcs[] */
static unsigned int count;

/**
 * fsl_bc_open - open the driver
 */
static int fsl_bc_open(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);
	struct fsl_bc_data *bc = &bcs[minor];

	filp->private_data = bc;

	return 0;
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
 * find_console_handle - find the byte channel handle to use for the console
 *
 * One of the byte channels could be used as the console, so we want to
 * skip it for this driver.
 */
static int find_console_handle(void)
{
	struct device_node *np;
	const char *sprop;
	const uint32_t *iprop;

	np = of_find_node_by_name(NULL, "aliases");
	if (!np)
		return -1;

	sprop = of_get_property(np, "stdout", NULL);
	if (!sprop)
		return -1;

	/* We don't care what the aliased node is actually called.  We only
	 * care if it's compatible with "fsl,hv-byte-channel", because that
	 * indicates that it's a byte channel node.
	 */
	np = of_find_node_by_path(sprop);
	if (!np) {
		pr_warning("fsl-bc: stdout node '%s' does not exist\n", sprop);
		return -1;
	}

	/* Is it a byte channel? */
	if (!of_device_is_compatible(np, "fsl,hv-byte-channel-handle"))
		return -1;

	iprop = of_get_property(np, "reg", NULL);
	if (!iprop) {
		pr_err("fsl-bc: no 'reg' property in %s node\n", np->name);
		return -1;
	}

	pr_info("fsl-bc: skipping console byte channel %u\n", *iprop);

	return *iprop;
}

static const struct file_operations fsl_bc_fops = {
	.owner = THIS_MODULE,
	.open = fsl_bc_open,
	.poll = fsl_bc_poll,
	.read = fsl_bc_read,
	.write = fsl_bc_write,
};

static struct class *fsl_bc_class;
static dev_t dev_id;
static struct cdev cdev;

/**
 * fsl_bc_init - Freescale hypervisor byte channel driver initialization
 *
 * This function is called when this module is loaded.
 */
static int __init fsl_bc_init(void)
{
	struct device_node *np;
	const uint32_t *reg;
	struct fsl_bc_data *bc;
	int stdout;
	unsigned int i;
	int ret;

	pr_info("Freescale hypervisor byte channel driver\n");

	if (!has_fsl_hypervisor()) {
		pr_info("fsl-bc: no hypervisor found\n");
		return -ENODEV;
	}

	stdout = find_console_handle();

	/* Count the number of byte channels */
	for_each_compatible_node(np, NULL, "fsl,hv-byte-channel-handle") {
		reg = of_get_property(np, "reg", NULL);
		if (reg && (*reg != stdout))
			count++;
	}
	if (!count) {
		pr_info("fsl-bc: no byte channels\n");
		return -ENODEV;
	}

	bcs = kzalloc(count * sizeof(struct fsl_bc_data), GFP_KERNEL);
	if (!bcs) {
		pr_err("fsl-bc: out of memory\n");
		return -ENOMEM;
	}

	ret = alloc_chrdev_region(&dev_id, 0, count, "fsl-bc");
	if (ret < 0) {
		pr_err("fsl-bc: unable to register char device\n");
		goto error_nomem;
	}

	/* Create our class in sysfs */
	fsl_bc_class = class_create(THIS_MODULE, "fsl-bc");

	i = 0;
	for_each_compatible_node(np, NULL, "fsl,hv-byte-channel-handle") {
		bc = &bcs[i];
		init_waitqueue_head(&bc->rx_wait);
		init_waitqueue_head(&bc->tx_wait);

		reg = of_get_property(np, "reg", NULL);
		if (!reg) {
			pr_err("fsl-bc: no 'reg' property in %s node\n",
				np->name);
			continue;
		}
		if (*reg == stdout)
			/* Skip the stdout byte channel */
			continue;

		bc->handle = *reg;

		bc->rx_irq = irq_of_parse_and_map(np, 0);
		if (bc->rx_irq == NO_IRQ) {
			pr_err("fsl-bc: no 'interrupts' property in %s node\n",
				np->name);
			continue;
		}
		ret = request_irq(bc->rx_irq, fsl_bc_rx_isr, 0, np->name, bc);
		if (ret < 0) {
			pr_err("fsl-bc: could not request rx irq %u\n",
			       bc->rx_irq);
			continue;
		}

		bc->tx_irq = irq_of_parse_and_map(np, 1);
		if (bc->tx_irq == NO_IRQ) {
			pr_err("fsl-bc: no 'interrupts' property in %s node\n",
				np->name);
			continue;
		}
		ret = request_irq(bc->tx_irq, fsl_bc_tx_isr, 0, np->name, bc);
		if (ret < 0) {
			pr_err("fsl-bc: could not request tx irq %u\n",
			       bc->tx_irq);
			continue;
		}

		/* Create the 'dev' entry in sysfs */
		bc->dev_id = MKDEV(MAJOR(dev_id), MINOR(dev_id) + i);
		bc->dev = device_create(fsl_bc_class, NULL, bc->dev_id, bc,
					"bc%u", bc->handle);
		if (IS_ERR(bc->dev)) {
			pr_err("fsl-bc: could not register byte channel %u\n",
				bc->handle);
			continue;
		}

		pr_info("fsl-bc: registered byte channel %u\n", bc->handle);

		i++;
	}

	cdev_init(&cdev, &fsl_bc_fops);
	cdev.owner = THIS_MODULE;

	ret = cdev_add(&cdev, dev_id, count);
	if (ret < 0) {
		pr_err("fsl-bc: could not add cdev\n");
		goto error;
	}

	return 0;

error:
	for (i = 0; i < count; i++) {
		device_destroy(fsl_bc_class, bcs[i].dev_id);
		free_irq(bcs[i].rx_irq, &bcs[i]);
		free_irq(bcs[i].tx_irq, &bcs[i]);
	}

	unregister_chrdev_region(dev_id, count);

error_nomem:
	kfree(bcs);

	return ret;
}


/**
 * fsl_bc_exit - Freescale hypervisor byte channel driver termination
 *
 * This function is called when this driver is unloaded.
 */
static void __exit fsl_bc_exit(void)
{
	unsigned int i;

	cdev_del(&cdev);

	for (i = 0; i < count; i++) {
		device_destroy(fsl_bc_class, bcs[i].dev_id);
		free_irq(bcs[i].rx_irq, &bcs[i]);
		free_irq(bcs[i].tx_irq, &bcs[i]);
	}

	unregister_chrdev_region(dev_id, count);

	kfree(bcs);
}

module_init(fsl_bc_init);
module_exit(fsl_bc_exit);

MODULE_AUTHOR("Timur Tabi <timur@freescale.com>");
MODULE_DESCRIPTION("Freescale hypervisor byte channel driver");
MODULE_LICENSE("GPL");
