/* Copyright (C) 2008-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MAC_H
#define __MAC_H

#include <linux/device.h>	/* struct device, BUS_ID_SIZE */
#include <linux/if_ether.h>	/* ETH_ALEN */
#include <linux/phy.h>		/* phy_interface_t, struct phy_device */

#include "fsl_fman.h"		/* struct port_device */

enum {DTSEC, XGMAC};

struct mac_device {
	struct device		*dev;
	void			*priv;
	uint8_t			 cell_index;
	struct resource		*res;
	void			*vaddr;
	uint8_t			 addr[ETH_ALEN];
	bool			 promisc;

	struct fm		*fm_dev;
	struct fm_port		*port_dev[2];

	phy_interface_t		 phy_if;
	u32			 if_support;
	bool			 link;
	bool			 half_duplex;
	uint16_t		 speed;
	uint16_t		 max_speed;
	struct device_node	*phy_node;
	char			fixed_bus_id[MII_BUS_ID_SIZE + 3];
	struct device_node	*tbi_node;
	struct phy_device	*phy_dev;
	void			*fm;

	int (*init_phy)(struct net_device *net_dev);
	int (*init)(struct mac_device *mac_dev);
	int (*start)(struct mac_device *mac_dev);
	int (*stop)(struct mac_device *mac_dev);
	int (*change_promisc)(struct mac_device *mac_dev);
	int (*uninit)(struct mac_device *mac_dev);
};

static inline void * __attribute((nonnull)) macdev_priv(const struct mac_device *mac_dev)
{
	return (void *)mac_dev + sizeof(*mac_dev);
}

extern const char	*mac_driver_description;
extern const size_t	 mac_sizeof_priv[];
extern void (*const mac_setup[])(struct mac_device *mac_dev);

#endif	/* __MAC_H */
