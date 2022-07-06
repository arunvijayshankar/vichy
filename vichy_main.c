/*
 * vichy_main.c The bare virtual character driver module (vichy)
 *
 * This source code is a minimal version of the 
 * Simple Character Utility for Loading Localities (scull) char 
 * module, which comes from the book "Linux Device Drivers" 
 * by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk */
#include <linux/slab.h>		/* kmalloc */
#include <linux/errno.h>	/* Error codes */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/semaphore.h>
#include <linux/types.h>	/* size_t */
#include <linux/fs.h>		/* Lots of things */
#include <linux/cdev.h>

#include <asm/uaccess.h>	/* copy_*_user */

#include "vichy.h"		/* local definitions */


/* Parameters that can be configured during load time */
int vichy_major =   VICHY_MAJOR;
int vichy_minor =   0;
int vichy_nr_devs = VICHY_NR_DEVS;
int vichy_quantum = VICHY_QUANTUM;

module_param(vichy_major, int, S_IRUGO);
module_param(vichy_minor, int, S_IRUGO);
module_param(vichy_nr_devs, int, S_IRUGO);
module_param(vichy_quantum, int, S_IRUGO);

MODULE_AUTHOR("Arun Vijayshankar");
MODULE_LICENSE("Dual BSD/GPL");

struct vichy_dev *vichy_devices; /* Allocated in vichy_init_module */

/* Empty out vichy device */
int vichy_trim(struct vichy_dev *dev) {
	struct vichy_qdata *next, *dptr;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = vichy_quantum;
	dev->data = NULL;
	return 0;
}

/* Open and release */
int vichy_open(struct inode *inode, struct file *filp) {
	struct vichy_dev *dev;
	
	dev = container_of(inode->i_cdev, struct vichy_dev, cdev);
	filp->private_data = dev; /* Store the device in private_data for other methods
					that might need it. */

	/* Trim the device to zero if it was opened as write-only */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		vichy_trim(dev);
	}
	return 0;
}

int vichy_release(struct inode *inode, struct file *filp) {
	return 0;
}

/* Follow the list */
struct vichy_qdata * vichy_follow(struct vichy_dev *dev, int n) {
	struct vichy_qdata *qd = dev->data;

	/* Allocate first qdata if needed */
	if (!qd) {
		qd = dev->data = kmalloc(sizeof(struct vichy_qdata), GFP_KERNEL);
		if (qd == NULL) {
			return NULL;
		}
		memset(qd, 0, sizeof(struct vichy_qdata));
	}

	/* Follow the list */
	while(n--) {
		if (!qd->next) {
			qd->next = kmalloc(sizeof(struct vichy_qdata), GFP_KERNEL);
			if (qd->next == NULL) {
				return NULL;
			}
			memset(qd, 0, sizeof(struct vichy_qdata));
		}
		qd = qd->next;
		continue;
	}
	return qd;
}

/* Data Management: read and write */
ssize_t vichy_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos) {
	struct vichy_dev *dev = filp->private_data;
	struct vichy_qdata *dptr; /* First list item */
	int quantum = dev->quantum;
	int item, q_pos, rest;
	ssize_t retval = 0;

	if (*f_pos >= dev->size) goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* Find list item, and offset in quantum */
	item = (long)*f_pos / quantum;
	rest = (long)*f_pos % quantum;
	q_pos = rest % quantum;

	/* Follow the list to the right item */
	dptr = vichy_follow(dev, item);

	if (dptr == NULL || !dptr->data)
		goto out; /* don't fill holes */

	/* Read only up to end of quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if(copy_to_user(buf, dptr->data + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:
    return retval;
}

ssize_t vichy_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos) {
	struct vichy_dev *dev = filp->private_data;
	struct vichy_qdata *dptr; 
	int quantum = dev->quantum;
	int item, q_pos, rest;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	/* Find list item, and offset in quantum */
	item = (long)*f_pos / quantum;
	rest = (long)*f_pos % quantum;
	q_pos = rest % quantum;

	/* Follow the list to the right item */
	dptr = vichy_follow(dev, item);

	if (dptr == NULL)
		goto out;

	if (!dptr->data) {
		dptr->data = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, quantum);
	}

	/* Read only up to end of quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if(copy_from_user(dptr->data + q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

	/* update the size */
	if (dev->size < *f_pos)
		dev->size = *f_pos;

  out:
    return retval;
}

struct file_operations vichy_fops = {
	.owner = THIS_MODULE,
	.read = vichy_read,
	.write = vichy_write,
	.open = vichy_open,
	.release = vichy_release,
};

void vichy_cleanup_module(void) {
	int i;

	/* get dev number from major and minor numbers*/
	dev_t first = MKDEV(vichy_major, vichy_minor);

	/* Free char dev entries */
	if (vichy_devices) {
		for (i = 0; i < vichy_nr_devs; i++) {
			vichy_trim(vichy_devices + i);
			cdev_del(&vichy_devices[i].cdev);
		}
		kfree(vichy_devices);
	}

	/* Unregirster memory region */
	unregister_chrdev_region(first, vichy_nr_devs);

}

static void vichy_setup_cdev(struct vichy_dev *dev, int index) {
	int err, devno = MKDEV(vichy_major, vichy_minor + index);
	cdev_init(&dev->cdev, &vichy_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &vichy_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding vichy%d", err, index);
	}
}

int vichy_init_module(void) {
	int res, i;
	dev_t dev = 0;
	/* Asking for a dynamic major number, unless specified otherwise at load time*/
	if (vichy_major) {
		dev = MKDEV(vichy_major, vichy_minor);
		res = register_chrdev_region(dev, vichy_nr_devs,
				"vichy");
	} else {
		res = alloc_chrdev_region(&dev, vichy_minor,
				vichy_nr_devs, "vichy");
		vichy_major = MAJOR(dev);
	}
	if (res < 0) {
		printk(KERN_WARNING "vichy: can't get major number %d\n", vichy_major);
		return res;
	}

	/* Allocate devices - number can be specified at load time */
	vichy_devices = kmalloc(vichy_nr_devs*sizeof(struct vichy_dev), GFP_KERNEL);
	if (!vichy_devices) {
		res = -ENOMEM;
		goto fail;
	}
	memset(vichy_devices, 0, vichy_nr_devs*sizeof(struct vichy_dev));

	/* Initialize char devices*/
	for (i = 0; i < vichy_nr_devs; i++) {
		vichy_devices[i].quantum = vichy_quantum;
		vichy_setup_cdev(&vichy_devices[i], i);
	}

	return 0; /* success */

  fail:
    vichy_cleanup_module();
    return res;
}

module_init(vichy_init_module);
module_exit(vichy_cleanup_module);

