/*
 * Freescale P4080DS MDIO bus
 * provides proper gpio muxing for the MDIO buses on the P4080 DS
 *
 * Author: Andy Fleming <afleming@freescale.com>
 *
 * Copyright (c) 2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

struct p4080ds_mdio {
	struct mii_bus *real_bus;
	u32 *gpio_reg;
	u32 gpio_value;
	u32 gpio_mask;
};

/* Set the GPIO mux, and then write the MDIO regs */
int p4080ds_mdio_write(struct mii_bus *bus, int port_addr, int dev_addr,
			int regnum, u16 value)
{
	struct p4080ds_mdio *priv = bus->priv;

	/* Write the GPIO regs to select this bus */
	clrsetbits_be32(priv->gpio_reg, priv->gpio_mask, priv->gpio_value);

	/* Write through to the attached MDIO bus */
	return priv->real_bus->write(priv->real_bus, port_addr, dev_addr,
					regnum, value);
}

/* Set the GPIO muxing, and then read from the MDIO bus */
int p4080ds_mdio_read(struct mii_bus *bus, int port_addr, int dev_addr,
			int regnum)
{
	struct p4080ds_mdio *priv = bus->priv;

	/* Write the GPIO regs to select this bus */
	clrsetbits_be32(priv->gpio_reg, priv->gpio_mask, priv->gpio_value);

	return priv->real_bus->read(priv->real_bus, port_addr, dev_addr,
					regnum);
}


/* Reset the MIIM registers, and wait for the bus to free */
static int p4080ds_mdio_reset(struct mii_bus *bus)
{
	struct p4080ds_mdio *priv = bus->priv;

	mutex_lock(&bus->mdio_lock);
	priv->real_bus->reset(priv->real_bus);
	mutex_unlock(&bus->mdio_lock);

	return 0;
}


static int p4080ds_mdio_probe(struct of_device *ofdev,
		const struct of_device_id *match)
{
	struct device_node *np = ofdev->node;
	struct mii_bus *new_bus;
	struct p4080ds_mdio *priv;
	struct device_node *mdio, *gpio;
	struct of_device *ofmdiodev;
	u64 reg;
	int i;
	const u32 *addr;
	const u32 *val;
	int err = 0;

	if (!of_device_is_available(np))
		return -ENODEV;

	new_bus = mdiobus_alloc();
	if (NULL == new_bus)
		return -ENOMEM;

	new_bus->name = "Freescale P4080DS MDIO Bus",
	new_bus->read = &p4080ds_mdio_read,
	new_bus->write = &p4080ds_mdio_write,
	new_bus->reset = &p4080ds_mdio_reset,

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		err = -ENOMEM;
		goto err_priv_alloc;
	}

	new_bus->priv = priv;

	mdio = of_parse_phandle(np, "fsl,mdio-handle", 0);

	if (mdio == NULL) {
		printk(KERN_ERR "Could not find real MDIO bus for %s\n",
			new_bus->id);
		err = -ENODEV;
		goto err_no_mdio_node;
	}

	ofmdiodev = of_find_device_by_node(mdio);

	if (!ofmdiodev) {
		printk(KERN_ERR "No of_device for MDIO node %s\n", mdio->name);
		err = -ENODEV;
		goto err_no_mdio_dev;
	}

	of_node_put(mdio);

	priv->real_bus = dev_get_drvdata(&ofmdiodev->dev);

	if (!priv->real_bus) {
		printk(KERN_ERR "The MDIO bus has no ofdev!\n");
		err = -ENODEV;
		goto err_no_ofdev;
	}

	new_bus->irq = kcalloc(PHY_MAX_ADDR, sizeof(int), GFP_KERNEL);

	if (NULL == new_bus->irq) {
		err = -ENOMEM;
		goto err_irq_alloc;
	}

	for (i = 0; i < PHY_MAX_ADDR; i++)
		new_bus->irq[i] = PHY_POLL;

	new_bus->parent = &ofdev->dev;
	dev_set_drvdata(&ofdev->dev, new_bus);

	/* Find the GPIO register pointer */
	gpio = of_find_compatible_node(NULL, NULL, "fsl,qoriq-gpio");

	if (!gpio) {
		err = -ENODEV;
		goto err_no_gpio;
	}

	addr = of_get_address(gpio, 0, NULL, NULL);
	if (!addr) {
		err = -ENODEV;
		goto err_no_gpio_addr;
	}

	reg = of_translate_address(gpio, addr);

	priv->gpio_reg = ioremap(reg, sizeof(*priv->gpio_reg));

	if (!priv->gpio_reg) {
		err = -ENOMEM;
		goto err_ioremap;
	}

	/* Grab the value to write to the GPIO */
	val = of_get_property(np, "fsl,muxval", NULL);

	if (!val) {
		printk(KERN_ERR "No mux value found for %s\n", np->full_name);
		err = -ENODEV;
		goto err_get_muxval;
	}

	if (of_device_is_compatible(np, "fsl,p4080ds-mdio")) {
		priv->gpio_mask = 0xc0000000;
		priv->gpio_value = (*val << 30);
	} else if (of_device_is_compatible(np, "fsl,p4080ds-xmdio")) {
		priv->gpio_mask = 0x30000000;
		priv->gpio_value = (*val << 28);
	}

	sprintf(new_bus->id, "%s@%d", np->name, *val);

	err = of_mdiobus_register(new_bus, np);

	if (err) {
		printk(KERN_ERR "%s: Cannot register as MDIO bus\n",
				new_bus->name);
		goto err_registration;
	}

	return 0;

err_registration:
err_get_muxval:
	iounmap(priv->gpio_reg);
err_ioremap:
err_no_gpio_addr:
err_no_gpio:
	kfree(new_bus->irq);
err_irq_alloc:
err_no_ofdev:
err_no_mdio_dev:
err_no_mdio_node:
	kfree(priv);
err_priv_alloc:
	kfree(new_bus);

	return err;
}


static int p4080ds_mdio_remove(struct of_device *ofdev)
{
	struct device *device = &ofdev->dev;
	struct mii_bus *bus = dev_get_drvdata(device);

	mdiobus_unregister(bus);

	dev_set_drvdata(device, NULL);

	bus->priv = NULL;
	mdiobus_free(bus);

	return 0;
}

static struct of_device_id p4080ds_mdio_match[] = {
	{
		.compatible = "fsl,p4080ds-mdio",
	},
	{
		.compatible = "fsl,p4080ds-xmdio",
	},
};

static struct of_platform_driver p4080ds_mdio_driver = {
	.name = "p4080ds_mdio",
	.probe = p4080ds_mdio_probe,
	.remove = p4080ds_mdio_remove,
	.match_table = p4080ds_mdio_match,
};

int __init p4080ds_mdio_init(void)
{
	return of_register_platform_driver(&p4080ds_mdio_driver);
}

void p4080ds_mdio_exit(void)
{
	of_unregister_platform_driver(&p4080ds_mdio_driver);
}
subsys_initcall_sync(p4080ds_mdio_init);
module_exit(p4080ds_mdio_exit);
