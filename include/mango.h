/*
 * Mango hypervisor interface definitions.
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

#ifndef __MANGO_H__
#define __MANGO_H__

/* Mango API */
unsigned int mango_unlock(unsigned char *token);

/* Data Channel */
unsigned int mango_dc_open(unsigned int ch, unsigned int dest);
unsigned int mango_dc_close(unsigned int ch);
unsigned int mango_dc_write(unsigned int ch, const unsigned char *p, unsigned int len);
unsigned int mango_dc_read(unsigned int ch, unsigned char *p, unsigned int len);
unsigned int mango_dc_tx_free_space(unsigned int ch);
unsigned int mango_dc_reset(unsigned int ch);
unsigned int mango_dc_set_mode(unsigned int ch, unsigned int mode);

/* Partition Management */
unsigned int mango_get_partition_id(void);
unsigned int mango_partition_reset(void);
unsigned int mango_get_partition_run_time(void);

/* Watchdog */
unsigned int mango_watchdog_start(void);
unsigned int mango_watchdog_ping(void);
unsigned int mango_watchdog_set_timeout(unsigned int timeout);

/* Networking */
unsigned int mango_net_open(unsigned int iface);
unsigned int mango_net_tx(unsigned int iface,
			  unsigned int dest,
			  const unsigned char *p,
			  unsigned int len);
unsigned int mango_net_rx(unsigned int iface,
			  unsigned char *p,
			  unsigned int len);
unsigned int mango_net_close(unsigned int iface);
unsigned int mango_net_set_mode(unsigned int iface, unsigned int mode);
unsigned int mango_net_get_rx_size(unsigned int iface);
unsigned int mango_net_reset(unsigned int iface);

#endif /* __MANGO_H__ */
