/** @file
 * Freescale Hypervisor Management Driver
 *
 * This driver contains functions to support the Freescale hypervisor.
 */
/* Copyright (C) 2008-2009 Freescale Semiconductor, Inc.
 * Author: Timur Tabi <timur@freescale.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/reboot.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/fsl_hcalls.h>

#include <linux/fsl_hypervisor.h>

/**
 * ioctl_restart: ioctl interface for FSL_HV_IOCTL_PARTITION_RESTART
 *
 * Restart a running partition
 */
static long ioctl_restart(struct fsl_hv_ioctl_restart __user *p)
{
	struct fsl_hv_ioctl_restart param;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_restart)))
		return -EFAULT;

	param.ret = fh_partition_restart(param.partition);

	if (copy_to_user(&p->ret, &param.ret, sizeof(__u32)))
		return -EFAULT;

	return 0;
}

/**
 * ioctl_status: ioctl interface for FSL_HV_IOCTL_PARTITION_STATUS
 *
 * Query the status of a partition
 */
static long ioctl_status(struct fsl_hv_ioctl_status __user *p)
{
	struct fsl_hv_ioctl_status param;
	u32 status;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_status)))
		return -EFAULT;

	param.ret = fh_partition_get_status(param.partition, &status);
	if (!param.ret)
		param.status = status;

	if (copy_to_user(p, &param, sizeof(struct fsl_hv_ioctl_status)))
		return -EFAULT;

	return 0;
}

/**
 * ioctl_start: ioctl interface for FSL_HV_IOCTL_PARTITION_START
 *
 * Start a stopped partition.
 */
static long ioctl_start(struct fsl_hv_ioctl_start __user *p)
{
	struct fsl_hv_ioctl_start param;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_start)))
		return -EFAULT;

	param.ret = fh_partition_start(param.partition, param.entry_point);

	if (copy_to_user(&p->ret, &param.ret, sizeof(__u32)))
		return -EFAULT;

	return 0;
}

/**
 * ioctl_stop: ioctl interface for FSL_HV_IOCTL_PARTITION_STOP
 *
 * Stop a running partition
 */
static long ioctl_stop(struct fsl_hv_ioctl_stop __user *p)
{
	struct fsl_hv_ioctl_stop param;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_stop)))
		return -EFAULT;

	param.ret = fh_partition_stop(param.partition);

	if (copy_to_user(&p->ret, &param.ret, sizeof(__u32)))
		return -EFAULT;

	return 0;
}

/**
 * ioctl_memcpy: ioctl interface for FSL_HV_IOCTL_MEMCPY
 *
 * The FH_MEMCPY hypercall takes an array of address/address/size structures
 * to represent the data being copied.  As a convenience to the user, this
 * ioctl takes a user-create buffer and a pointer to a guest physically
 * contiguous buffer in the remote partition, and creates the
 * address/address/size array for the hypercall.
 */
static long ioctl_memcpy(struct fsl_hv_ioctl_memcpy __user *p)
{
	struct fsl_hv_ioctl_memcpy param;

	struct page **pages = NULL;
	void *sg_list_unaligned = NULL;
	struct fh_sg_list *sg_list = NULL;

	unsigned int nr_pages;
	unsigned long lb_offset; /* Offset within a page of the local buffer */

	unsigned int i;
	long ret = 0;
	phys_addr_t remote_paddr; /* The next address in the remote buffer */
	uint32_t count; /* The number of bytes left to copy */

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_memcpy)))
		return -EFAULT;

	/* One partition must be local, the other must be remote.  In other
	   words, if source and target are both -1, or are both not -1, then
	   return an error. */
	if ((param.source == -1) == (param.target == -1))
		return -EINVAL;

	/*
	 * The array of pages returned by get_user_pages() covers only
	 * page-aligned memory.  Since the user buffer is probably not
	 * page-aligned, we need to handle the discrepancy.
	 *
	 * We calculate the offset within a page of the S/G list, and make
	 * adjustments accordingly.  This will result in a page list that looks
	 * like this:
	 *
	 *      ----    <-- first page starts before the buffer
	 *     |    |
	 *     |////|-> ----
	 *     |////|  |    |
	 *      ----   |    |
	 *             |    |
	 *      ----   |    |
	 *     |////|  |    |
	 *     |////|  |    |
	 *     |////|  |    |
	 *      ----   |    |
	 *             |    |
	 *      ----   |    |
	 *     |////|  |    |
	 *     |////|  |    |
	 *     |////|  |    |
	 *      ----   |    |
	 *             |    |
	 *      ----   |    |
	 *     |////|  |    |
	 *     |////|-> ----
	 *     |    |   <-- last page ends after the buffer
	 *      ----
	 *
	 * The distance between the start of the first page and the start of the
	 * buffer is lb_offset.  The hashed (///) areas are the parts of the
	 * page list that contain the actual buffer.
	 *
	 * The advantage of this approach is that the number of pages is
	 * equal to the number of entries in the S/G list that we give to the
	 * hypervisor.
	 */
	lb_offset = param.local_vaddr & (PAGE_SIZE - 1);
	nr_pages = (param.count + lb_offset + PAGE_SIZE - 1) >> PAGE_SHIFT;

	/* Allocate the buffers we need */

	/* pages is an array of struct page pointers that's initialized by
	   get_user_pages() */
	pages = kzalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		pr_debug("fsl-hv: could not allocate page list\n");
		return -ENOMEM;
	}

	/* sg_list is the list of fh_sg_list objects that we pass to the
	   hypervisor */
	sg_list_unaligned = kmalloc(nr_pages * sizeof(struct fh_sg_list) +
		sizeof(struct fh_sg_list) - 1, GFP_KERNEL);
	if (!sg_list_unaligned) {
		pr_debug("fsl-hv: could not allocate S/G list\n");
		return -ENOMEM;
	}
	sg_list = PTR_ALIGN(sg_list_unaligned, sizeof(struct fh_sg_list));

	/* Get the physical addresses of the source buffer */
	down_read(&current->mm->mmap_sem);
	ret = get_user_pages(current, current->mm,
		param.local_vaddr - lb_offset, nr_pages,
		(param.source == -1) ? READ : WRITE,
		0, pages, NULL);
	up_read(&current->mm->mmap_sem);

	if (ret != nr_pages) {
		/* get_user_pages() failed */
		pr_debug("fsl-hv: could not lock source buffer\n");
		ret = -EACCES;
		goto exit;
	}

	/* reset ret here */
	ret = 0;

	/* Build the fh_sg_list[] array.  The first page is special
	   because it's misaligned.*/
	if (param.source == -1) {
		sg_list[0].source = page_to_phys(pages[0]) + lb_offset;
		sg_list[0].target = param.remote_paddr;
	} else {
		sg_list[0].source = param.remote_paddr;
		sg_list[0].target = page_to_phys(pages[0]) + lb_offset;
	}
	sg_list[0].size = min_t(uint64_t, param.count, PAGE_SIZE - lb_offset);

	remote_paddr = param.remote_paddr + sg_list[0].size;
	count = param.count - sg_list[0].size;

	for (i = 1; i < nr_pages; i++) {
		if (param.source == -1) {
			/* local to remote */
			sg_list[i].source = page_to_phys(pages[i]);
			sg_list[i].target = remote_paddr;
		} else {
			/* remote to local */
			sg_list[i].source = remote_paddr;
			sg_list[i].target = page_to_phys(pages[i]);
		}
		sg_list[i].size = min_t(uint64_t, count, PAGE_SIZE);

		remote_paddr += sg_list[i].size;
		count -= sg_list[i].size;
	}

	param.ret = fh_partition_memcpy(param.source, param.target,
		virt_to_phys(sg_list), nr_pages);

exit:
	if (pages) {
		for (i = 0; i < nr_pages; i++)
			if (pages[i])
				page_cache_release(pages[i]);
	}

	kfree(sg_list_unaligned);
	kfree(pages);

	if (!ret)
		if (copy_to_user(&p->ret, &param.ret, sizeof(__u32)))
			return -EFAULT;

	return ret;
}

/**
 * ioctl_doorbell: ioctl interface for FSL_HV_IOCTL_DOORBELL
 *
 * Ring a doorbell
 */
static long ioctl_doorbell(struct fsl_hv_ioctl_doorbell __user *p)
{
	struct fsl_hv_ioctl_doorbell param;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_doorbell)))
		return -EFAULT;

	param.ret = fh_partition_send_dbell(param.doorbell);

	if (copy_to_user(&p->ret, &param.ret, sizeof(__u32)))
		return -EFAULT;

	return 0;
}

static char *strdup_from_user(const char __user *ustr, size_t max)
{
	size_t len;
	char *str;

	len = strnlen_user(ustr, max);
	if (len > max)
		return ERR_PTR(-ENAMETOOLONG);

	str = kmalloc(len, GFP_KERNEL);
	if (!str)
		return ERR_PTR(-ENOMEM);

	if (copy_from_user(str, ustr, len))
		return ERR_PTR(-EFAULT);

	return str;
}

static long ioctl_dtprop(struct fsl_hv_ioctl_prop __user *p, int set)
{
	struct fsl_hv_ioctl_prop param;
	char __user *upath, *upropname;
	void __user *upropval;
	char *path = NULL, *propname = NULL;
	void *propval = NULL;
	int ret = 0;

	/* Get the parameters from the user */
	if (copy_from_user(&param, p, sizeof(struct fsl_hv_ioctl_prop)))
		return -EFAULT;

	upath = (char __user *)(uintptr_t)param.path;
	upropname = (char __user *)(uintptr_t)param.propname;
	upropval = (void __user *)(uintptr_t)param.propval;

	path = strdup_from_user(upath, FH_DTPROP_MAX_PATHLEN);
	if (IS_ERR(path)) {
		ret = PTR_ERR(path);
		goto out;
	}

	propname = strdup_from_user(upropname, FH_DTPROP_MAX_PATHLEN);
	if (IS_ERR(propname)) {
		ret = PTR_ERR(propname);
		goto out;
	}

	if (param.proplen > FH_DTPROP_MAX_PROPLEN) {
		ret = -EINVAL;
		goto out;
	}

	propval = kmalloc(param.proplen, GFP_KERNEL);
	if (!propval) {
		ret = -ENOMEM;
		goto out;
	}

	if (set) {
		if (copy_from_user(propval, upropval, param.proplen)) {
			ret = -EFAULT;
			goto out;
		}

		param.ret = fh_partition_set_dtprop(param.handle,
						    virt_to_phys(path),
						    virt_to_phys(propname),
						    virt_to_phys(propval),
						    param.proplen);
	} else {
		param.ret = fh_partition_get_dtprop(param.handle,
						    virt_to_phys(path),
						    virt_to_phys(propname),
						    virt_to_phys(propval),
						    &param.proplen);

		if (param.ret == 0) {
			if (copy_to_user(upropval, propval, param.proplen) ||
			    put_user(param.proplen, &p->proplen)) {
				ret = -EFAULT;
				goto out;
			}
		}
	}

	if (put_user(param.ret, &p->ret))
		ret = -EFAULT;

out:
	kfree(path);
	kfree(propval);
	kfree(propname);

	return ret;
}

/**
 * fsl_hv_ioctl: ioctl main entry point
 */
static long fsl_hv_ioctl(struct file *file, unsigned int cmd,
                         unsigned long argaddr)
{
	union fsl_hv_ioctl_param __user *arg =
		(union fsl_hv_ioctl_param __user *)argaddr;
	long ret;

	/* Make sure the application is called the right driver. */
	if (_IOC_TYPE(cmd) != 0) {
		pr_debug("fsl-hv: ioctl type %u should be 0\n", _IOC_TYPE(cmd));
		return -EINVAL;
	}

	/* Make sure the application set the direction flag correctly. */
	if (_IOC_DIR(cmd) != (_IOC_READ | _IOC_WRITE)) {
		pr_debug("fsl-hv: ioctl direction should be _IOWR\n");
		return -EINVAL;
	}

	/* Make sure the application is passing the right structure to us. */
	if (_IOC_SIZE(cmd) < sizeof(union fsl_hv_ioctl_param)) {
		pr_debug("fsl-hv: ioctl size %u is too small (should be %u)\n",
			 _IOC_SIZE(cmd), sizeof(union fsl_hv_ioctl_param));
		return -EINVAL;
	}

	switch (_IOC_NR(cmd)) {
	case FSL_HV_IOCTL_PARTITION_RESTART:
		ret = ioctl_restart(&arg->restart);
		break;
	case FSL_HV_IOCTL_PARTITION_GET_STATUS:
		ret = ioctl_status(&arg->status);
		break;
	case FSL_HV_IOCTL_PARTITION_START:
		ret = ioctl_start(&arg->start);
		break;
	case FSL_HV_IOCTL_PARTITION_STOP:
		ret = ioctl_stop(&arg->stop);
		break;
	case FSL_HV_IOCTL_MEMCPY:
		ret = ioctl_memcpy(&arg->memcpy);
		break;
	case FSL_HV_IOCTL_DOORBELL:
		ret = ioctl_doorbell(&arg->doorbell);
		break;
	case FSL_HV_IOCTL_GETPROP:
		ret = ioctl_dtprop(&arg->prop, 0);
		break;
	case FSL_HV_IOCTL_SETPROP:
		ret = ioctl_dtprop(&arg->prop, 1);
		break;
	default:
		pr_debug("fsl-hv: unknown ioctl %u\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

/* Linked list of processes that have us open */
struct list_head db_list;

/* spinlock for db_list */
static DEFINE_SPINLOCK(db_list_lock);

/* The size of the doorbell event queue.  This must be a power of two. */
#define QSIZE	16

/* Returns the next head/tail pointer, wrapping around the queue if necessary */
#define nextp(x) (((x) + 1) & (QSIZE - 1))

/* Per-open data structure */
struct doorbell_queue {
	struct list_head list;
	spinlock_t lock;
	wait_queue_head_t wait;
	unsigned int head;
	unsigned int tail;
	uint32_t q[QSIZE];
};

/**
 * fsl_hv_isr - interrupt handler for all doorbells
 * @param irq - the IRQ (a.k.a. receive handle)
 *
 * We use the same interrupt handler for all doorbells.  Whenever a doorbell
 * is rung, and we receive an interrupt, we just put the handle for that
 * doorbell (passed to us as *data) into all of the queues.
 *
 */
static irqreturn_t fsl_hv_isr(int irq, void *data)
{
	struct doorbell_queue *dbq;
	unsigned long flags;

	/* Prevent another core from modifying db_list */
	spin_lock_irqsave(&db_list_lock, flags);

	list_for_each_entry(dbq, &db_list, list) {
		if (dbq->head != nextp(dbq->tail)) {
			dbq->q[dbq->tail] = (uint32_t) (uintptr_t) data;
			/* This memory barrier eliminates the need to grab
			 * the spinlock for dbq.
			 */
			smp_wmb();
			dbq->tail = nextp(dbq->tail);
			wake_up_interruptible(&dbq->wait);
		}
	}

	spin_unlock_irqrestore(&db_list_lock, flags);

	return IRQ_HANDLED;
}
/**
 * fsl_hv_poll - returns a bitmask indicating whether a read will block
 *
 * @return unsigned int
 */
static unsigned int fsl_hv_poll(struct file *filp, struct poll_table_struct *p)
{
	struct doorbell_queue *dbq = filp->private_data;
	unsigned long flags;
	unsigned int mask;

	spin_lock_irqsave(&dbq->lock, flags);

	poll_wait(filp, &dbq->wait, p);
	mask = (dbq->head == dbq->tail) ? 0 : (POLLIN | POLLRDNORM);

	spin_unlock_irqrestore(&dbq->lock, flags);

	return mask;
}

/**
 * fsl_hv_read - return the handles for any incoming doorbells
 *
 * If there are doorbell handles in the queue for this open instance, then
 * return them to the caller as an array of 32-bit integers.  Otherwise,
 * block until there is at least one handle to return.
 */
static ssize_t fsl_hv_read(struct file *filp, char __user *buf, size_t len,
			   loff_t *off)
{
	struct doorbell_queue *dbq = filp->private_data;
	uint32_t __user *p = (uint32_t __user *) buf; /* for put_user() */
	unsigned long flags;
	ssize_t count = 0;

	/* Make sure we stop when the user buffer is full. */
	while (len >= sizeof(uint32_t)) {
		uint32_t dbell;	/* Local copy of doorbell queue data */

		spin_lock_irqsave(&dbq->lock, flags);

		/* If the queue is empty, then either we're done or we need
		 * to block.  If the application specified O_NONBLOCK, then
		 * we return the appropriate error code.
		 */
		if (dbq->head == dbq->tail) {
			spin_unlock_irqrestore(&dbq->lock, flags);
			if (count)
				break;
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;
			if (wait_event_interruptible(dbq->wait,
						     dbq->head != dbq->tail))
				return -ERESTARTSYS;
			continue;
		}

		/* Even though we have an smp_wmb() in the ISR, the core
		 * might speculatively execute the "dbell = ..." below while
		 * it's evaluating the if-statement above.  In that case, the
		 * value put into dbell could be stale if the core accepts the
		 * speculation. To prevent that, we need a read memory barrier
		 * here as well.
		 */
		smp_rmb();

		/* Copy the data to a temporary local buffer, because
		 * we can't call copy_to_user() from inside a spinlock
		 */
		dbell = dbq->q[dbq->head];
		dbq->head = nextp(dbq->head);

		spin_unlock_irqrestore(&dbq->lock, flags);

		if (put_user(dbell, p))
			return -EFAULT;
		p++;
		count += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	return count;
}

/**
 * fsl_hv_open - open the driver
 *
 * Open the driver and prepare for reading doorbells.
 *
 * Every time an application opens the driver, we create a doorbell queue
 * for that file handle.  This queue is used for any incoming doorbells.
 */
static int fsl_hv_open(struct inode *inode, struct file *filp)
{
	struct doorbell_queue *dbq;
	unsigned long flags;
	int ret = 0;

	dbq = kzalloc(sizeof(struct doorbell_queue), GFP_KERNEL);
	if (!dbq) {
		pr_err("fsl-hv: out of memory\n");
		return -ENOMEM;
	}

	spin_lock_init(&dbq->lock);
	init_waitqueue_head(&dbq->wait);

	spin_lock_irqsave(&db_list_lock, flags);
	list_add(&dbq->list, &db_list);
	spin_unlock_irqrestore(&db_list_lock, flags);

	filp->private_data = dbq;

	return ret;
}

/**
 * fsl_hv_close - close the driver
 */
static int fsl_hv_close(struct inode *inode, struct file *filp)
{
	struct doorbell_queue *dbq = filp->private_data;
	unsigned long flags;

	int ret = 0;

	spin_lock_irqsave(&db_list_lock, flags);
	list_del(&dbq->list);
	spin_unlock_irqrestore(&db_list_lock, flags);

	kfree(dbq);

	return ret;
}

static const struct file_operations fsl_hv_fops = {
	.owner =            THIS_MODULE,
	.open = 	    fsl_hv_open,
	.release = 	    fsl_hv_close,
	.poll =		    fsl_hv_poll,
	.read =		    fsl_hv_read,
	.unlocked_ioctl =   fsl_hv_ioctl,
};

static struct miscdevice fsl_hv_misc_dev = {
	MISC_DYNAMIC_MINOR,
	"fsl-hv",
	&fsl_hv_fops
};

static DECLARE_WORK(power_off, (work_func_t) kernel_power_off);

static irqreturn_t fsl_hv_shutdown_isr(int irq, void *data)
{
	/* Security feature - if the caller doesn't pass the right 'data',
	 * then we reject it.
	 */
	if (data == fsl_hv_shutdown_isr)
		schedule_work(&power_off);

	/* We should never get here */
	return IRQ_NONE;
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
 * fsl_hypervisor_init: Freescale hypervisor management driver init
 *
 * This function is called when this module is loaded.
 *
 * Register ourselves as a miscellaneous driver.  This will register the
 * fops structure and create the right sysfs entries for udev.
 */
static int __init fsl_hypervisor_init(void)
{
	struct device_node *np;
	int ret;

	pr_info("Freescale hypervisor management driver\n");

	if (!has_fsl_hypervisor()) {
		pr_info("fsl-hv: no hypervisor found\n");
		return -ENODEV;
	}

	ret = misc_register(&fsl_hv_misc_dev);
	if (ret) {
		pr_err("fsl-hv: cannot register device\n");
		return ret;
	}

	INIT_LIST_HEAD(&db_list);

	for_each_compatible_node(np, NULL, "fsl,hv-doorbell-receive-handle") {
		unsigned int irq;
		const uint32_t *handle;

		handle = of_get_property(np, "interrupts", NULL);
		irq = irq_of_parse_and_map(np, 0);
		if (!handle || (irq == NO_IRQ)) {
			pr_err("fsl-hv: no 'interrupts' property in %s node\n",
				np->name);
			continue;
		}

		if (of_device_is_compatible(np, "fsl,hv-shutdown-doorbell"))
			/* The shutdown doorbell gets its own ISR */
			ret = request_irq(irq, fsl_hv_shutdown_isr, 0,
					  np->name, fsl_hv_shutdown_isr);
		else
			ret = request_irq(irq, fsl_hv_isr, 0, np->name,
					  (void *) (uintptr_t) *handle);
		if (ret < 0) {
			pr_err("fsl-hv: could not request irq %u\n", irq);
			continue;
		}

		pr_info("fsl-hv: registered handler for doorbell %u\n",
			*handle);
	}

	return 0;
}

/**
 * fsl_hypervisor_exit: Freescale hypervisor management driver termination
 *
 * This function is called when this driver is unloaded.
 */
static void __exit fsl_hypervisor_exit(void)
{
	struct device_node *np;

	for_each_compatible_node(np, NULL, "fsl,hv-doorbell-receive-handle") {
		unsigned int irq;
		const uint32_t *handle;

		irq = irq_of_parse_and_map(np, 0);
		if (irq == NO_IRQ)
			continue;

		handle = of_get_property(np, "interrupts", NULL);
		if (!handle)
			continue;

		free_irq(irq, (void *) (uintptr_t) *handle);
	}

	misc_deregister(&fsl_hv_misc_dev);
}

module_init(fsl_hypervisor_init);
module_exit(fsl_hypervisor_exit);

MODULE_AUTHOR("Timur Tabi <timur@freescale.com>");
MODULE_DESCRIPTION("Freescale hypervisor management driver");
MODULE_LICENSE("GPL");
