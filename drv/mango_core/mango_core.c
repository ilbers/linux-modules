/*
 * Mango hypervisor interface.
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

#include <linux/kernel.h>
#include <linux/module.h>

/* Mango hypercall identifiers, should be the same as in Mango core */
#define MANGO_HVC_DC_OPEN		0x10
#define MANGO_HVC_DC_WRITE		0x11
#define MANGO_HVC_DC_READ		0x12
#define MANGO_HVC_DC_CLOSE		0x13
#define MANGO_HVC_DC_TX_FREE_SPACE	0x14
#define MANGO_HVC_DC_RESET		0x15

#define MANGO_HVC_PARTITION_ID		0x20
#define MANGO_HVC_PARTITION_RESET	0x21

#define MANGO_HVC_WD_START		0x30
#define MANGO_HVC_WD_STOP		0x31
#define MANGO_HVC_WD_PING		0x32
#define MANGO_HVC_WD_SET_TIMEOUT	0x33

#define mango_hypervisor_call_0(nr)					\
({									\
	register unsigned int _ret;					\
	asm volatile ("hvc	%1\r\n"					\
		      "mov	%0, r0\r\n"				\
		      : "=r"(_ret)					\
		      : "I" (nr)					\
		      : "cc");						\
	_ret;								\
})

#define mango_hypervisor_call_1(nr, arg1)				\
({									\
	register unsigned int _ret;					\
	asm volatile ("mov	r0, %2\r\n"				\
		      "hvc	%1\r\n"					\
		      "mov	%0, r0\r\n"				\
		      : "=r"(_ret)					\
		      : "I" (nr), "r" (arg1)				\
		      : "cc");						\
	_ret;								\
})

#define mango_hypervisor_call_2(nr, arg1, arg2)				\
({									\
	register unsigned int _ret;					\
	asm volatile ("mov	r0, %2\r\n"				\
		      "mov	r1, %3\r\n"				\
		      "hvc	%1\r\n"					\
		      "mov	%0, r0\r\n"				\
		      : "=r"(_ret)					\
		      : "I" (nr), "r" (arg1), "r" (arg2)		\
		      : "cc");						\
	_ret;								\
})

#define mango_hypervisor_call_3(nr, arg1, arg2, arg3)			\
({									\
	register unsigned int _ret;					\
	asm volatile ("mov	r0, %2\r\n"				\
		      "mov	r1, %3\r\n"				\
		      "mov	r2, %4\r\n"				\
		      "hvc	%1\r\n"					\
		      "mov	%0, r0\r\n"				\
		      : "=r"(_ret)					\
		      : "I" (nr), "r" (arg1), "r" (arg2), "r" (arg3)	\
		      : "cc");						\
	_ret;								\
})

/***********************************/
/*     Mango Data Channel API      */
/***********************************/

unsigned int mango_dc_open(unsigned int ch, unsigned int dest)
{
	unsigned int ret;

	ret = mango_hypervisor_call_2(MANGO_HVC_DC_OPEN, ch, dest);

	return ret;
}
EXPORT_SYMBOL(mango_dc_open);

unsigned int mango_dc_write(unsigned int ch, const unsigned char *p, unsigned int len)
{
	unsigned int ret;

	ret =  mango_hypervisor_call_3(MANGO_HVC_DC_WRITE,
				       ch,
				       (unsigned int)p,
				       len);

	return ret;
}
EXPORT_SYMBOL(mango_dc_write);

unsigned int mango_dc_read(unsigned int ch, unsigned char *p, unsigned int len)
{
	unsigned int ret;

	ret =  mango_hypervisor_call_3(MANGO_HVC_DC_READ,
				       ch,
				       (unsigned int)p,
				       len);

	return ret;
}
EXPORT_SYMBOL(mango_dc_read);

unsigned int mango_dc_close(unsigned int ch)
{
	unsigned int ret;

	ret = mango_hypervisor_call_1(MANGO_HVC_DC_CLOSE, ch);

	return ret;
}
EXPORT_SYMBOL(mango_dc_close);

unsigned int mango_dc_tx_free_space(unsigned int ch)
{
	unsigned int ret;

	ret = mango_hypervisor_call_1(MANGO_HVC_DC_TX_FREE_SPACE, ch);

	return ret;
}
EXPORT_SYMBOL(mango_dc_tx_free_space);

unsigned int mango_dc_reset(unsigned int ch)
{
	unsigned int ret;

	ret = mango_hypervisor_call_1(MANGO_HVC_DC_RESET, ch);

	return ret;
}
EXPORT_SYMBOL(mango_dc_reset);

/*******************************************/
/*     Mango Partition Management API      */
/*******************************************/

unsigned int mango_get_partition_id(void)
{
	unsigned int ret;

	ret = mango_hypervisor_call_0(MANGO_HVC_PARTITION_ID);

	return ret;
}
EXPORT_SYMBOL(mango_get_partition_id);

unsigned int mango_partition_reset(void)
{
	unsigned int ret;

	ret = mango_hypervisor_call_0(MANGO_HVC_PARTITION_RESET);

	return ret;
}
EXPORT_SYMBOL(mango_partition_reset);

/*******************************/
/*     Mango Watchdog API      */
/*******************************/
unsigned int mango_watchdog_start(void)
{
	unsigned int ret;

	ret = mango_hypervisor_call_0(MANGO_HVC_WD_START);

	return ret;
}
EXPORT_SYMBOL(mango_watchdog_start);

unsigned int mango_watchdog_stop(void)
{
	unsigned int ret;

	ret = mango_hypervisor_call_0(MANGO_HVC_WD_STOP);

	return ret;
}
EXPORT_SYMBOL(mango_watchdog_stop);

unsigned int mango_watchdog_ping(void)
{
	unsigned int ret;

	ret = mango_hypervisor_call_0(MANGO_HVC_WD_PING);

	return ret;
}
EXPORT_SYMBOL(mango_watchdog_ping);

unsigned int mango_watchdog_set_timeout(unsigned int timeout)
{
	unsigned int ret;

	ret = mango_hypervisor_call_1(MANGO_HVC_WD_SET_TIMEOUT, timeout);

	return ret;
}
EXPORT_SYMBOL(mango_watchdog_set_timeout);

MODULE_AUTHOR("Alexander Smirnov");
MODULE_DESCRIPTION("Mango Hypervisor Interface");
MODULE_LICENSE("GPL");
