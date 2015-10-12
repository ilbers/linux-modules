/*
 * Linux driver for Mango Watchdog.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

#include <mango.h>

#define CLASS_NAME	"mango_wd"	/* Device class name */
#define DEVICE_NAME	"wd"		/* Device name as it appears in /proc/devices */

struct wd_dev_t {
	int           is_open;		/* Device open flag */
	int           major;
	struct class  *class_wd;
	struct device *dev;
};

static struct wd_dev_t wd_dev;

static int wd_open(struct inode *inode, struct file *filep)
{
	if (wd_dev.is_open)
		return -EBUSY;

	wd_dev.is_open = 1;

	return 0;
}

static int wd_release(struct inode *inode, struct file *filep)
{
	wd_dev.is_open = 0;

	return 0;
}

static ssize_t wd_write(struct file *filep,
			const char *buff,
			size_t len,
			loff_t *off)
{
	int ret;

	ret = mango_watchdog_ping();
	if (ret)
		return 0;
	else
		return 1;
}

static struct file_operations wd_fops = {
	.write   = wd_write,
	.open    = wd_open,
	.release = wd_release
};

void wd_module_exit(void)
{
	return;
}

int wd_module_init(void)
{
	void *ptr_err;

	/* Register watchdog character device */
        wd_dev.major = register_chrdev(0, DEVICE_NAME, &wd_fops);
	if (wd_dev.major < 0) {
		printk(KERN_ALERT "mango_wd: register watchdog failed with %d\n", wd_dev.major);
		goto err;
	}

	/* Create watchdog device class */
	wd_dev.class_wd = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ptr_err = wd_dev.class_wd)) {
		printk(KERN_ALERT "mango_wd: failed to create device class\n");
		goto err_unreg;
	}

	/* Create watchdog device */
	wd_dev.dev = device_create(wd_dev.class_wd,
				   NULL,
				   MKDEV(wd_dev.major, 0),
				   NULL,
				   DEVICE_NAME);
	if (IS_ERR(ptr_err = wd_dev.dev)) {
		printk(KERN_ALERT "mango_wd: failed to create device class\n");
		goto err_destroy;
	}

	wd_dev.is_open = 0;

	mango_watchdog_start();

	return 0;

err_destroy:
	class_destroy(wd_dev.class_wd);
err_unreg:
	unregister_chrdev(wd_dev.major, DEVICE_NAME);
err:
	return -EINVAL;
}

module_init(wd_module_init);
module_exit(wd_module_exit);

MODULE_AUTHOR("Alexander Smirnov");
MODULE_DESCRIPTION("Mango Watchdog");
MODULE_LICENSE("GPL");
