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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/netdevice.h>

#include "dpa-common.h"
#include "dpa.h"
#include "mac.h"

#include "error_ext.h"	/* GET_ERROR_TYPE, E_OK */
#include "fm_mac_ext.h"

#define MAC_DESCRIPTION "FSL FMan MAC API based driver"

MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Emil Medve <Emilian.Medve@Freescale.com>");

MODULE_DESCRIPTION(MAC_DESCRIPTION);

struct mac_priv_s {
	t_Handle	mac;
};

const char	*mac_driver_description __initconst = MAC_DESCRIPTION;
const size_t	 mac_sizeof_priv[] __devinitconst = {
	[DTSEC] = sizeof(struct mac_priv_s),
	[XGMAC] = sizeof(struct mac_priv_s)
};

static const e_EnetInterface _100[] __devinitconst =
{
	[PHY_INTERFACE_MODE_MII]	= e_ENET_MODE_MII_100,
	[PHY_INTERFACE_MODE_RMII]	= e_ENET_MODE_RMII_100
};

static const e_EnetInterface _1000[] __devinitconst =
{
	[PHY_INTERFACE_MODE_GMII]	= e_ENET_MODE_GMII_1000,
	[PHY_INTERFACE_MODE_SGMII]	= e_ENET_MODE_SGMII_1000,
	[PHY_INTERFACE_MODE_TBI]	= e_ENET_MODE_TBI_1000,
	[PHY_INTERFACE_MODE_RGMII]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_ID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_RXID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_TXID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RTBI]	= e_ENET_MODE_RTBI_1000
};

static e_EnetInterface __devinit __cold __attribute__((nonnull))
macdev2enetinterface(const struct mac_device *mac_dev)
{
	switch (mac_dev->max_speed) {
	case SPEED_100:
		return _100[mac_dev->phy_if];
	case SPEED_1000:
		return _1000[mac_dev->phy_if];
	case SPEED_10000:
		return e_ENET_MODE_XGMII_10000;
	default:
		return e_ENET_MODE_MII_100;
	}
}

static void mac_exception(t_Handle _mac_dev, e_FmMacExceptions exception)
{
	struct mac_device	*mac_dev;

	mac_dev = (typeof(mac_dev))_mac_dev;

	cpu_dev_dbg(mac_dev->dev, "%s:%s() -> %d\n", __file__, __func__, exception);
}

static int __devinit __cold init(struct mac_device *mac_dev)
{
	int					_errno;
	t_Error				err;
	struct mac_priv_s	*priv;
	t_FmMacParams		param;
	uint32_t			version;

	priv = (typeof(priv))macdev_priv(mac_dev);

	param.baseAddr =  (typeof(param.baseAddr))(uintptr_t)devm_ioremap(
		mac_dev->dev, mac_dev->res->start, 0x2000);
	param.enetMode	= macdev2enetinterface(mac_dev);
	memcpy(&param.addr, mac_dev->addr, min(sizeof(param.addr),
		sizeof(mac_dev->addr)));
	param.macId			= mac_dev->cell_index;
	param.h_Fm 			= (t_Handle)mac_dev->fm;
	param.mdioIrq		= NO_IRQ;
	param.f_Exception	= mac_exception;
	param.f_Event		= mac_exception;
	param.h_App			= mac_dev;

	priv->mac = FM_MAC_Config(&param);
	if (unlikely(priv->mac == NULL)) {
		cpu_dev_err(mac_dev->dev,
				"%s:%hu:%s(): FM_MAC_Config() failed\n",
				__file__, __LINE__, __func__);
		_errno = -EINVAL;
		goto _return;
	}

	err = FM_MAC_ConfigMaxFrameLength(priv->mac, 9600);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_ConfigMaxFrameLength() = 0x%08x\n",
			__file__, __LINE__, __func__, err);
		goto _return_fm_mac_free;
	}

	err = FM_MAC_ConfigPadAndCrc(priv->mac, true);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_ConfigPadAndCrc() = 0x%08x\n",
			__file__, __LINE__, __func__, err);
		goto _return_fm_mac_free;
	}

	if (macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000) {
		err = FM_MAC_ConfigHalfDuplex(priv->mac, mac_dev->half_duplex);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			cpu_dev_err(mac_dev->dev,
				"%s:%hu: FM_MAC_ConfigHalfDuplex() = 0x%08x\n",
				__file__, __LINE__, err);
			goto _return_fm_mac_free;
		}
	}
	else  {
		err = FM_MAC_ConfigResetOnInit(priv->mac, true);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			cpu_dev_err(mac_dev->dev, "%s:%hu:%s(): FM_MAC_ConfigResetOnInit() = 0x%08x\n",
				    __file__, __LINE__, __func__, err);
			goto _return_fm_mac_free;
		}
	}

	err = FM_MAC_Init(priv->mac);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_Init() = 0x%08x\n",
			__file__, __LINE__, __func__, err);
		goto _return_fm_mac_free;
	}

	err = FM_MAC_GetVesrion(priv->mac, &version);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_GetVesrion() = 0x%08x\n",
			__file__, __LINE__, __func__, err);
		goto _return_fm_mac_free;
	}
	cpu_dev_info(mac_dev->dev, "FMan %s version: 0x%08x\n",
		((macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000) ?
			"dTSEC" : "XGEC"), version);

	goto _return;


_return_fm_mac_free:
	err = FM_MAC_Free(priv->mac);
	if (unlikely(-GET_ERROR_TYPE(err) < 0))
		cpu_dev_err(mac_dev->dev,
				"%s:%hu:%s(): FM_MAC_Free() = 0x%08x\n",
				__file__, __LINE__, __func__, err);
_return:
	return _errno;
}

static int __cold start(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;
	struct phy_device *phy_dev = mac_dev->phy_dev;

	err = FM_MAC_Enable(((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		cpu_dev_err(mac_dev->dev,
				"%s:%hu:%s(): FM_MAC_Enable() = 0x%08x\n",
				__file__, __LINE__, __func__, err);

	if (phy_dev) {
		if (macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000)
			phy_start(phy_dev);
		else if (phy_dev->drv->read_status)
			phy_dev->drv->read_status(phy_dev);
	}

	return _errno;
}

static int __cold stop(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;

	if (mac_dev->phy_dev &&
		(macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000))
		phy_stop(mac_dev->phy_dev);

	err = FM_MAC_Disable(((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
				e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		cpu_dev_err(mac_dev->dev,
				"%s:%hu:%s(): FM_MAC_Disable() = 0x%08x\n",
				__file__, __LINE__, __func__, err);

	return _errno;
}

static int __cold change_promisc(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;

	err = FM_MAC_SetPromiscuous(
			((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			mac_dev->promisc = !mac_dev->promisc);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_SetPromiscuous() = 0x%08x\n",
			__file__, __LINE__, __func__, err);

	return _errno;
}

static void adjust_link(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	struct phy_device *phy_dev = mac_dev->phy_dev;
	int	 _errno;
	t_Error	 err;

	if (!phy_dev->link)
		return;

	err = FM_MAC_AdjustLink(
			((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			phy_dev->speed, phy_dev->duplex);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		cpu_dev_err(mac_dev->dev,
			"%s:%hu:%s(): FM_MAC_AdjustLink() = 0x%08x\n",
			__file__, __LINE__, __func__, err);

	return;
}

/* Initializes driver's PHY state, and attaches to the PHY.
 * Returns 0 on success.
 */
static int dtsec_init_phy(struct net_device *net_dev)
{
	struct dpa_priv_s	*priv;
	struct mac_device	*mac_dev;
	struct phy_device	*phy_dev;

	priv = netdev_priv(net_dev);
	mac_dev = priv->mac_dev;

	if (!mac_dev->phy_node)
		phy_dev = phy_connect(net_dev, mac_dev->fixed_bus_id,
				&adjust_link, 0, mac_dev->phy_if);
	else
		phy_dev = of_phy_connect(net_dev, mac_dev->phy_node,
				&adjust_link, 0, mac_dev->phy_if);
	if (unlikely(phy_dev == NULL) || IS_ERR(phy_dev)) {
		cpu_netdev_err(net_dev,
				"%s:%hu:%s(): Could not connect to PHY %s\n",
				__file__, __LINE__, __func__,
				mac_dev->phy_node ?
					mac_dev->phy_node->full_name :
					mac_dev->fixed_bus_id);
		return phy_dev == NULL ? -ENODEV : PTR_ERR(phy_dev);
	}

	/* Remove any features not supported by the controller */
	phy_dev->supported &= priv->mac_dev->if_support;
	phy_dev->advertising = phy_dev->supported;

	priv->mac_dev->phy_dev = phy_dev;

	return 0;
}

static int xgmac_init_phy(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	struct phy_device *phy_dev;

	if (!mac_dev->phy_node)
		phy_dev = phy_attach(net_dev, mac_dev->fixed_bus_id, 0,
				mac_dev->phy_if);
	else
		phy_dev = of_phy_attach(net_dev, mac_dev->phy_node, 0,
				mac_dev->phy_if);
	if (unlikely(phy_dev == NULL) || IS_ERR(phy_dev)) {
		cpu_netdev_err(net_dev,
				"%s:%hu:%s(): Could not attach to PHY %s\n",
				__file__, __LINE__, __func__,
				mac_dev->phy_node ?
					mac_dev->phy_node->full_name :
					mac_dev->fixed_bus_id);
		return phy_dev == NULL ? -ENODEV : PTR_ERR(phy_dev);
	}

	phy_dev->supported &= priv->mac_dev->if_support;
	phy_dev->advertising = phy_dev->supported;

	mac_dev->phy_dev = phy_dev;

	return 0;
}

static int __cold uninit(struct mac_device *mac_dev)
{
	int			 _errno, __errno;
	t_Error			 err;
	const struct mac_priv_s	*priv;

	priv = (typeof(priv))macdev_priv(mac_dev);

	err = FM_MAC_Disable(priv->mac, e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		cpu_dev_err(mac_dev->dev, "%s:%hu:%s(): FM_MAC_Disable() = 0x%08x\n",
			    __file__, __LINE__, __func__, err);

	err = FM_MAC_Free(priv->mac);
	__errno = -GET_ERROR_TYPE(err);
	if (unlikely(__errno < 0)) {
		cpu_dev_err(mac_dev->dev, "%s:%hu:%s(): FM_MAC_Free() = 0x%08x\n",
			    __file__, __LINE__, __func__, err);
		if (_errno < 0)
			_errno = __errno;
	}

	return _errno;
}

static void __devinit __cold setup_dtsec(struct mac_device *mac_dev)
{
	mac_dev->init_phy	= dtsec_init_phy;
	mac_dev->init		= init;
	mac_dev->start		= start;
	mac_dev->stop		= stop;
	mac_dev->change_promisc	= change_promisc;
	mac_dev->uninit		= uninit;
}

static void __devinit __cold setup_xgmac(struct mac_device *mac_dev)
{
	mac_dev->init_phy	= xgmac_init_phy;
	mac_dev->init		= init;
	mac_dev->start		= start;
	mac_dev->stop		= stop;
	mac_dev->change_promisc	= change_promisc;
	mac_dev->uninit		= uninit;
}

void (*const mac_setup[])(struct mac_device *mac_dev) __devinitconst = {
	[DTSEC] = setup_dtsec,
	[XGMAC] = setup_xgmac
};
