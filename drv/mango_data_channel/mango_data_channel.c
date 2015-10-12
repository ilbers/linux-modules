/*
 * Linux driver for Mango Data Channel.
 *
 * Copyright (c) 2014-2015 ilbers GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <mango.h>
#include <ring_buffer.h>

#define DC_IRQ_NR		130		/* Base Mango Data Channel physical IRQ */

#define CLASS_NAME		"mango_dc"	/* Device class name */
#define DEVICE_NAME		"dc"		/* Device name as it appears in /proc/devices */
#define DC_BUFFER_SIZE		256		/* Ring buffer size to store incomming data */

RING_BUFFER(dc_buffer_t, unsigned char, DC_BUFFER_SIZE);

struct dc_dev_t {
	int               major;		/* Device major number */
	int               is_open;		/* Device open flag */
	int               irq;			/* IRQ line assigned to the device */
	int               ch;			/* Mango data channel identifier */
	int               dest;			/* Destination partition for channel */
	spinlock_t        lock;			/* Synchronization */
	struct list_head  list;			/* Device list entry */
	wait_queue_head_t wq;			/* Waitqueue for I/O operations */
	struct device     *dev;
	dc_buffer_t       buff;			/* Internal device buffer */
};

/* Data Channel devices list */
static LIST_HEAD(dc_devs_list);

/* Number of Data Channel devices */
static int nr_devs = 1;

/* Destination partition for data channels */
static int dest_part = 1;

/* Data Channel device class */
struct class  *class_dc;

static irqreturn_t dc_mango_irq(int irq, void *data)
{
	int count;
	unsigned char buf[DC_BUFFER_SIZE];
	struct dc_dev_t *dev = data;

	spin_lock(&dev->lock);

	do {
		count = mango_dc_read(dev->ch, buf, DC_BUFFER_SIZE);
		if (count) {
			int i;

			for (i = 0; i < count; i++)
				RING_BUFFER_PUSH(dev->buff, buf[i]);
		}
	} while (count);

	spin_unlock(&dev->lock);

	wake_up_interruptible(&dev->wq);

	return IRQ_HANDLED;
}

static int dc_open(struct inode *inode, struct file *filep)
{
	struct dc_dev_t *dev;
	int minor;

	minor = iminor(inode);

	list_for_each_entry(dev, &dc_devs_list, list) {
		if (dev->ch == minor) {
			if (dev->is_open)
				return -EBUSY;

			dev->is_open = 1;
			filep->private_data = dev;

			printk("dc%d: data channel openned\n", minor);

			return 0;
		}
	}

	return -ENODEV;
}

static int dc_release(struct inode *inode, struct file *filep)
{
	struct dc_dev_t *dev = filep->private_data;

	dev->is_open = 0;

	return 0;
}

static ssize_t dc_read(struct file *filep,
		       char *buffer,
		       size_t length,
		       loff_t *offset)
{
	struct dc_dev_t *dev = filep->private_data;
	ssize_t bytes_read = 0;

	wait_event_interruptible(dev->wq, RING_BUFFER_FILL(dev->buff));

	while (RING_BUFFER_FILL(dev->buff) && length) {
		put_user(RING_BUFFER_POP(dev->buff), buffer++);
		bytes_read++;
		length--;
	}

	return bytes_read;
}

static ssize_t dc_write(struct file *filep,
			const char *buff,
			size_t len,
			loff_t *off)
{
	struct dc_dev_t *dev = filep->private_data;
	size_t size = (len > DC_BUFFER_SIZE) ? DC_BUFFER_SIZE : len;
	size_t count;

	count = mango_dc_write(dev->ch, buff, size);

	return count;	
}

static struct file_operations dc_fops = {
	.read    = dc_read,
	.write   = dc_write,
	.open    = dc_open,
	.release = dc_release
};

void dc_module_exit(void)
{
	struct dc_dev_t *dev, *tmp;

	list_for_each_entry_safe(dev, tmp, &dc_devs_list, list) {
		mango_dc_close(dev->ch);

		disable_irq(dev->irq);
		free_irq(dev->irq, (void*)dev);
		unregister_chrdev(dev->major, DEVICE_NAME);
		device_destroy(class_dc, MKDEV(dev->major, dev->ch));

		list_del(&dev->list);
		kfree(dev);
	}

	class_destroy(class_dc);
}

int dc_module_init(void)
{
	int i, ret;
	struct dc_dev_t *dev;
	void *ptr_err;

	/* Create watchdog device class */
	class_dc = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ptr_err = class_dc)) {
		printk(KERN_ALERT "mango_dc: failed to create device class\n");
		return -EINVAL;
	}

	for (i = 0; i < nr_devs; i++) {
		dev = kmalloc(sizeof(struct dc_dev_t), GFP_KERNEL);
		if (!dev) {
			printk(KERN_ALERT "mango_dc: failed to allocated dc#%d\n", i);
			goto out;
		}

	        ret = register_chrdev(0, DEVICE_NAME, &dc_fops);
		if (ret < 0) {
			printk(KERN_ALERT "mango_dc: register data channel device failed with %d\n",
			       ret);
			goto out_free;
		}

		dev->major   = ret;
		dev->is_open = 0;
		dev->dest    = dest_part;
		dev->irq     = DC_IRQ_NR + i;
		dev->ch      = i;
		RING_BUFFER_INIT(dev->buff);

		init_waitqueue_head(&dev->wq);
		spin_lock_init(&dev->lock);

		dev->dev = device_create(class_dc,
					 NULL,
					 MKDEV(dev->major, i),
					 NULL,
					 DEVICE_NAME "%d", i);
		if (IS_ERR(ptr_err = dev->dev)) {
			printk(KERN_ALERT "mango_wd: failed to create device class\n");
			goto out_unreg;
		}

		/* Setup Data Channel interface */
		ret = request_irq(dev->irq,
				  dc_mango_irq,
				  0,
				  DEVICE_NAME,
				  (void *)dev);
		if (ret) {
			printk(KERN_ALERT "mango_dc: failed to request IRQ for dc#%d\n", dev->ch);
			goto out_destroy;
		}

		disable_irq_nosync(dev->irq);
		enable_irq(dev->irq);

		ret = mango_dc_open(dev->ch, dev->dest);

		if (ret) {
			printk(KERN_ALERT "mango_dc: failed to open mango dc#%d\n", dev->ch);
			goto out_free_irq;
		}

		printk("mango_dc: dc#%d registered\n", dev->ch);

		list_add(&dev->list, &dc_devs_list);
	}

	return 0;

out_free_irq:
	disable_irq(dev->irq);
	free_irq(dev->irq, (void *)dev);
out_destroy:
	device_destroy(class_dc, MKDEV(dev->major, i));
out_unreg:
	unregister_chrdev(dev->major, DEVICE_NAME);
out_free:
	kfree(dev);
out:
	dc_module_exit();

	return -EINVAL;
}

module_init(dc_module_init);
module_exit(dc_module_exit);

module_param(nr_devs, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(nr_devs, "number of data channel to create");

MODULE_AUTHOR("Alexander Smirnov");
MODULE_DESCRIPTION("Mango Data Channel");
MODULE_LICENSE("GPL");
