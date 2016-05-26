/*
 * Mango hypervisor interface.
 *
 * Copyright (c) 2014-2016 ilbers GmbH
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
#define MANGO_HVC_AUTH			0x01

#define MANGO_HVC_DC_OPEN		0x10
#define MANGO_HVC_DC_WRITE		0x11
#define MANGO_HVC_DC_READ		0x12
#define MANGO_HVC_DC_CLOSE		0x13
#define MANGO_HVC_DC_TX_FREE_SPACE	0x14
#define MANGO_HVC_DC_RESET		0x15
#define MANGO_HVC_DC_SET_MODE		0x16

#define MANGO_HVC_PARTITION_ID		0x20
#define MANGO_HVC_PARTITION_RESET	0x21
#define MANGO_HVC_PARTITION_RUN_TIME	0x22

#define MANGO_HVC_WD_START		0x30
#define MANGO_HVC_WD_STOP		0x31
#define MANGO_HVC_WD_PING		0x32
#define MANGO_HVC_WD_SET_TIMEOUT	0x33

#define MANGO_HVC_CONSOLE_WRITE		0x40

#define MANGO_HVC_DEBUG			0x50

#define MANGO_HVC_NET_OPEN		0x61
#define MANGO_HVC_NET_SET_MODE		0x62
#define MANGO_HVC_NET_TX		0x63
#define MANGO_HVC_NET_RX		0x64
#define MANGO_HVC_NET_CLOSE		0x65
#define MANGO_HVC_NET_RX_SIZE		0x66
#define MANGO_HVC_NET_RESET		0x67

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

#define mango_hypervisor_call_4(nr, arg1, arg2, arg3, arg4)		\
({									\
	register unsigned int _ret;					\
	asm volatile ("mov	r0, %2\r\n"				\
		      "mov	r1, %3\r\n"				\
		      "mov	r2, %4\r\n"				\
		      "mov	r3, %5\r\n"				\
		      "hvc	%1\r\n"					\
		      "mov	%0, r0\r\n"				\
		      : "=r"(_ret)					\
		      : "I" (nr), "r" (arg1), "r" (arg2), "r" (arg3),	\
			"r" (arg4)					\
		      : "cc");						\
	_ret;								\
})

/* Mango passphrase */
static char *secure_token = 0;

/***********************************/
/*         Mango Core API          */
/***********************************/
unsigned int mango_unlock(unsigned char *token)
{
	return mango_hypervisor_call_1(MANGO_HVC_AUTH, token);
}
EXPORT_SYMBOL(mango_unlock);

/***********************************/
/*     Mango Data Channel API      */
/***********************************/
unsigned int mango_dc_open(unsigned int ch, unsigned int dest)
{
	return mango_hypervisor_call_2(MANGO_HVC_DC_OPEN, ch, dest);
}
EXPORT_SYMBOL(mango_dc_open);

unsigned int mango_dc_write(unsigned int ch, const unsigned char *p, unsigned int len)
{
	return mango_hypervisor_call_3(MANGO_HVC_DC_WRITE,
				       ch,
				       (unsigned int)p,
				       len);
}
EXPORT_SYMBOL(mango_dc_write);

unsigned int mango_dc_read(unsigned int ch, unsigned char *p, unsigned int len)
{
	return mango_hypervisor_call_3(MANGO_HVC_DC_READ,
				       ch,
				       (unsigned int)p,
				       len);
}
EXPORT_SYMBOL(mango_dc_read);

unsigned int mango_dc_close(unsigned int ch)
{
	return mango_hypervisor_call_1(MANGO_HVC_DC_CLOSE, ch);
}
EXPORT_SYMBOL(mango_dc_close);

unsigned int mango_dc_tx_free_space(unsigned int ch)
{
	return mango_hypervisor_call_1(MANGO_HVC_DC_TX_FREE_SPACE, ch);
}
EXPORT_SYMBOL(mango_dc_tx_free_space);

unsigned int mango_dc_reset(unsigned int ch)
{
	return mango_hypervisor_call_1(MANGO_HVC_DC_RESET, ch);
}
EXPORT_SYMBOL(mango_dc_reset);

unsigned int mango_dc_set_mode(unsigned int ch, unsigned int mode)
{
	return mango_hypervisor_call_2(MANGO_HVC_DC_SET_MODE, ch, mode);
}
EXPORT_SYMBOL(mango_dc_set_mode);

/*******************************************/
/*     Mango Partition Management API      */
/*******************************************/

unsigned int mango_get_partition_id(void)
{
	return mango_hypervisor_call_0(MANGO_HVC_PARTITION_ID);
}
EXPORT_SYMBOL(mango_get_partition_id);

unsigned int mango_partition_reset(void)
{
	return mango_hypervisor_call_0(MANGO_HVC_PARTITION_RESET);
}
EXPORT_SYMBOL(mango_partition_reset);

unsigned int mango_get_partition_run_time(void)
{
	return mango_hypervisor_call_0(MANGO_HVC_PARTITION_RUN_TIME);
}
EXPORT_SYMBOL(mango_get_partition_run_time);

/*******************************/
/*     Mango Watchdog API      */
/*******************************/
unsigned int mango_watchdog_start(void)
{
	return mango_hypervisor_call_0(MANGO_HVC_WD_START);
}
EXPORT_SYMBOL(mango_watchdog_start);

unsigned int mango_watchdog_ping(void)
{
	return mango_hypervisor_call_0(MANGO_HVC_WD_PING);
}
EXPORT_SYMBOL(mango_watchdog_ping);

unsigned int mango_watchdog_set_timeout(unsigned int timeout)
{
	return mango_hypervisor_call_1(MANGO_HVC_WD_SET_TIMEOUT, timeout);
}
EXPORT_SYMBOL(mango_watchdog_set_timeout);

/*********************************/
/*     Mango Networking API      */
/*********************************/
unsigned int mango_net_open(unsigned int iface)
{
	return mango_hypervisor_call_1(MANGO_HVC_NET_OPEN, iface);
}
EXPORT_SYMBOL(mango_net_open);

unsigned int mango_net_tx(unsigned int iface,
			  unsigned int dest,
			  const unsigned char *p,
			  unsigned int len)
{
	return mango_hypervisor_call_4(MANGO_HVC_NET_TX,
				       iface,
				       dest,
				       (unsigned int)p,
				       len);
}
EXPORT_SYMBOL(mango_net_tx);

unsigned int mango_net_rx(unsigned int iface,
			  unsigned char *p,
			  unsigned int len)
{
	return mango_hypervisor_call_3(MANGO_HVC_NET_RX,
				       iface,
				       (unsigned int)p,
				       (unsigned int)len);
}
EXPORT_SYMBOL(mango_net_rx);

unsigned int mango_net_close(unsigned int iface)
{
	return mango_hypervisor_call_1(MANGO_HVC_NET_CLOSE, iface);
}
EXPORT_SYMBOL(mango_net_close);

unsigned int mango_net_set_mode(unsigned int iface, unsigned int mode)
{
	return mango_hypervisor_call_2(MANGO_HVC_NET_SET_MODE, iface, mode);
}
EXPORT_SYMBOL(mango_net_set_mode);

unsigned int mango_net_get_rx_size(unsigned int iface)
{
	return mango_hypervisor_call_1(MANGO_HVC_NET_RX_SIZE, iface);
}
EXPORT_SYMBOL(mango_net_get_rx_size);

unsigned int mango_net_reset(unsigned int iface)
{
	return mango_hypervisor_call_1(MANGO_HVC_NET_RESET, iface);
}
EXPORT_SYMBOL(mango_net_reset);

int mango_core_init(void)
{
	unsigned int ret;

	ret = mango_unlock(secure_token);

	if (ret)
		printk("mango_core: invalid passphrase, aborting\n");

	return ret;
}

module_init(mango_core_init);

module_param(secure_token, charp, 0);
MODULE_PARM_DESC(secure_token, "Mango secure passphrase");

MODULE_AUTHOR("Alexander Smirnov");
MODULE_DESCRIPTION("Mango Hypervisor Interface");
MODULE_LICENSE("GPL");
