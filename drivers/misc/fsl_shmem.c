/** @file
 * Freescale Shared Memory Driver
 *
 * A driver to allow user applications to request memory to be shared with a
 * process on another core (typically on another OS).
 */
/* Copyright (C) 2008-2009 Freescale Semiconductor, Inc.
 * Authors: Andy Fleming <afleming@freescale.com>
 *          Timur Tabi <timur@freescale.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/of.h>

static struct region_list {
	struct list_head list;
	resource_size_t start;
	resource_size_t end;
	unsigned long prot;
} regions;

static int fsl_shmem_open(struct inode *inode, struct file *filp)
{
	filp->f_mapping->backing_dev_info = &directly_mappable_cdev_bdi;

	return 0;
}

static int fsl_shmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long addr = vma->vm_pgoff << PAGE_SHIFT;
	struct list_head *list;

	list_for_each(list, &regions.list) {
		struct region_list *region =
			list_entry(list, struct region_list, list);

		if ((addr >= region->start) &&
		    ((addr + size - 1) <= region->end)) {
			unsigned long prot = pgprot_val(vma->vm_page_prot);

			vma->vm_page_prot = __pgprot(prot | region->prot);

			if (remap_pfn_range(vma, vma->vm_start,	vma->vm_pgoff,
					    size, vma->vm_page_prot))
				return -EAGAIN;
			return 0;
		}
	}

	return -EINVAL;
}

static const struct file_operations shmem_fops = {
	.open		= fsl_shmem_open,
	.mmap		= fsl_shmem_mmap,
};

static struct miscdevice fsl_shmem_miscdev = {
	.name = "fsl-shmem",
	.fops = &shmem_fops,
	.minor = MISC_DYNAMIC_MINOR,
};

static int __init add_region(struct device_node *node, unsigned long prot)
{
	struct resource res;
	struct region_list *region;
	int ret;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		pr_warning("fsl-shmem: couldn't get shmem resource "
		       "for node %s\n", node->full_name);
		return 0;
	}

	region = kzalloc(sizeof(struct region_list), GFP_KERNEL);
	if (!region)
		return -ENOMEM;

	pr_info("fsl-shmem: registering address range %pR\n", &res);

	region->start = res.start;
	region->end = res.end;
	region->prot = prot;

	list_add(&region->list, &regions.list);

	return 0;
}

static int __init fsl_shmem_init(void)
{
	struct device_node *node;
	struct list_head *n, *list;
	int ret;

	pr_info("Freescale shared memory driver\n");

	INIT_LIST_HEAD(&regions.list);

	for_each_compatible_node(node, NULL, "fsl-shmem") {
		ret = add_region(node, _PAGE_COHERENT);
		if (ret)
			return ret;
	}

	for_each_compatible_node(node, NULL, "fsl-deco") {
		ret = add_region(node, _PAGE_NO_CACHE | _PAGE_GUARDED);
		if (ret)
			return ret;
	}

	if (list_empty(&regions.list))
		/* No shared memory regions found. */
		return 0;

	ret = misc_register(&fsl_shmem_miscdev);
	if (ret) {
		pr_info("fsl-shmem: failed to register misc device\n");
		goto error;
	}

	return 0;

error:
	list_for_each_safe(list, n, &regions.list) {
		struct region_list *region;

		region = list_entry(list, struct region_list, list);
		list_del(list);
		kfree(region);
	}

	return ret;
}

static void __exit fsl_shmem_exit(void)
{
	struct list_head *n, *list;

	misc_deregister(&fsl_shmem_miscdev);

	list_for_each_safe(list, n, &regions.list) {
		struct region_list *region;

		region = list_entry(list, struct region_list, list);
		list_del(list);
		kfree(region);
	}
}

module_init(fsl_shmem_init);
module_exit(fsl_shmem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("Freescale shared memory driver");
