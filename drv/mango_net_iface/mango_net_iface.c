/*
 * Linux driver for Mango cross-partition networking.
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

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/moduleparam.h>
#include <net/rtnetlink.h>

#include <mango.h>

#define DC_MANGO_CHANNEL	1
#define DC_MANGO_NET_IRQ	(130 + DC_MANGO_CHANNEL)
#define MANGO_NET_TARGET	1
#define MANGO_MAX_FRAME_SIZE	512

static int channel = 1;
static int max_interrupt_work = 20;

struct netdev_private {
	struct napi_struct      napi;
	struct net_device       *dev;
	struct net_device_stats stats;
};

static netdev_tx_t mango_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	unsigned char buf[MANGO_MAX_FRAME_SIZE + ETH_HLEN + 4];

	if (mango_dc_tx_free_space(DC_MANGO_CHANNEL) >= (skb->len + 4)) {
		memcpy(buf, &skb->len, 4);
		memcpy(buf + 4, skb->data, skb->len);

		mango_dc_write(DC_MANGO_CHANNEL, buf, skb->len + 4);

		np->stats.tx_packets++;
		np->stats.tx_bytes += skb->len;
	} else {
		printk(KERN_ALERT "error: no free space in dc channel, drop the packet\n");
	}

	dev_kfree_skb(skb);

	return NETDEV_TX_OK;
}

static int mango_dev_recv(struct net_device *dev, int *quota)
{
	struct netdev_private *np = netdev_priv(dev);
	struct sk_buff *skb;
	unsigned char *buf;
	int ret = 1;
	size_t n, size;

	n = mango_dc_read(DC_MANGO_CHANNEL, (void *)&size, 4);

	if (n == 0) {
		ret = 0;
		goto out;
	}

	if (n != 4) {
		printk(KERN_ALERT "error: dc sync failed, requested (%d) - got (%d)\n", 4, n);
		mango_dc_reset(DC_MANGO_CHANNEL);
		goto out;
	}

	buf = kmalloc(size, GFP_KERNEL);

	n = mango_dc_read(DC_MANGO_CHANNEL, buf, size);
	if (n != size) {
		printk(KERN_ALERT "error: dc sync failed, requested (%d) - got (%d)\n", size, n);
		mango_dc_reset(DC_MANGO_CHANNEL);
		goto out;
	}

	skb = netdev_alloc_skb_ip_align(dev, n);
	memcpy(skb_put(skb, size), buf, size);

	kfree(buf);

	np->stats.rx_packets++;
	np->stats.rx_bytes += n;

	skb->protocol = eth_type_trans(skb, dev);
	netif_receive_skb(skb);

out:
	return ret;
}

static int netdev_poll(struct napi_struct *napi, int budget)
{
	struct netdev_private *np = container_of(napi, struct netdev_private, napi);
	struct net_device *dev = np->dev;
	int quota = budget;

	while (mango_dev_recv(dev, &quota));

	napi_complete(napi);

	/* Restore IRQ signaling */
	mango_dc_set_mode(DC_MANGO_CHANNEL, DC_MODE_IRQ);

	return 0;
}

static irqreturn_t mango_dev_irq(int irq, void *data)
{
	struct net_device *dev = data;
	struct netdev_private *np = netdev_priv(dev);

	/* Disable IRQ signaling for incomming data */
	mango_dc_set_mode(DC_MANGO_CHANNEL, DC_MODE_POLLING);

	if (likely(napi_schedule_prep(&np->napi)))
		__napi_schedule(&np->napi);

	return IRQ_HANDLED;
}

static int mango_dev_init(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	int err;

	/* Setup Data Channel interface */
	err = request_irq(DC_MANGO_NET_IRQ,
			  mango_dev_irq,
			  0,
			  "mango_net",
			  (void *)dev);
	if (err) {
		printk(KERN_ALERT "mango_net: failed to request IRQ for dc\n");
		goto err;
	}

	disable_irq_nosync(DC_MANGO_NET_IRQ);
	enable_irq(DC_MANGO_NET_IRQ);

	napi_enable(&np->napi);
	netif_start_queue(dev);

	err = mango_dc_open(DC_MANGO_CHANNEL, MANGO_NET_TARGET);
	if (err) {
		printk(KERN_ALERT "mango_dc: failed to open mango data channel\n");
		goto err;
	}

	return 0;
err:
	disable_irq_nosync(DC_MANGO_NET_IRQ);
	free_irq(DC_MANGO_NET_IRQ, (void *)dev);
	return err;
}

static void mango_dev_uninit(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);

	netif_stop_queue(dev);
	napi_disable(&np->napi);

	mango_dc_close(DC_MANGO_CHANNEL);
	disable_irq_nosync(DC_MANGO_NET_IRQ);
	free_irq(DC_MANGO_NET_IRQ, (void *)dev);
}

static struct net_device_stats *mango_get_stats(struct net_device *dev)
{
	struct netdev_private *np = netdev_priv(dev);
	struct net_device_stats *nstat = &dev->stats;
	struct net_device_stats *stat = &np->stats;

	nstat->rx_packets = stat->rx_packets;
	nstat->rx_bytes   = stat->rx_bytes;

	nstat->tx_packets = stat->tx_packets;
	nstat->tx_bytes   = stat->tx_bytes;

	return nstat;
}

static const struct net_device_ops mango_netdev_ops = {
	.ndo_init	= mango_dev_init,
	.ndo_uninit	= mango_dev_uninit,
	.ndo_start_xmit	= mango_dev_xmit,
	.ndo_get_stats  = mango_get_stats,
};

static void mango_setup(struct net_device *dev)
{
	ether_setup(dev);

	/* Initialize the device structure. */
	dev->netdev_ops = &mango_netdev_ops;
	dev->destructor = free_netdev;

	/* Fill in device structure with ethernet-generic values. */
	dev->tx_queue_len = 0;
	dev->flags       &= ~IFF_MULTICAST;
	dev->features	 |= NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_TSO;
	dev->features	 |= NETIF_F_HW_CSUM | NETIF_F_HIGHDMA | NETIF_F_LLTX;

	/* This network driver works over Mango data-channel. The default
	 * buffer size is 2048 bytes. So to have a chance to store several
	 * network packets - reduce MTU. Or, change buffer size in hypervisor.
	 */
	dev->mtu = MANGO_MAX_FRAME_SIZE;

	eth_hw_addr_random(dev);
}

static struct rtnl_link_ops mango_link_ops __read_mostly = {
	.kind	= "mango",
	.setup	= mango_setup,
};

static int __init mango_init_one(void)
{
	struct net_device *dev_mango;
	struct netdev_private *np;
	int err;

	dev_mango = alloc_netdev(sizeof(*np), "mango%d", mango_setup);
	if (!dev_mango)
		return -ENOMEM;

	np = netdev_priv(dev_mango);
	np->dev = dev_mango;

	netif_napi_add(dev_mango, &np->napi, netdev_poll, max_interrupt_work);

	dev_mango->rtnl_link_ops = &mango_link_ops;
	err = register_netdevice(dev_mango);
	if (err < 0)
		goto err;

	return 0;

err:
	free_netdev(dev_mango);
	return err;
}

static int __init mango_init_module(void)
{
	int err = 0;

	rtnl_lock();
	err = __rtnl_link_register(&mango_link_ops);
	if (err < 0)
		goto out;

	err = mango_init_one();
	if (err < 0)
		__rtnl_link_unregister(&mango_link_ops);
out:
	rtnl_unlock();

	return err;
}

static void __exit mango_cleanup_module(void)
{
	rtnl_link_unregister(&mango_link_ops);
}

module_init(mango_init_module);
module_exit(mango_cleanup_module);

module_param(channel, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(channel, "data channel to use for transport");

MODULE_AUTHOR("Alexander Smirnov");
MODULE_DESCRIPTION("Mango Cross-Partition Networking");
MODULE_LICENSE("GPL");
