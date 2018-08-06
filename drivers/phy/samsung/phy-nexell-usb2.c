/*
 * Nexell SoC USB 1.1/2.0 PHY driver
 *
 * Copyright (C) 2016 Nexell Co., Ltd.
 * Author: Hyunseok Jung <hsjung@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy/phy.h>
#include <linux/regmap.h>
#include "phy-samsung-usb2.h"

/* Nexell USBHOST PHY registers */

#define SYSREG_USB0		0x90
#define SYSREG_USB1		0x94
#define SYSREG_USB2		0x98
#define SYSREG_USB3		0x128

#define SYSREG_USB2_POR_SHIFT	6
#define SYSREG_USB3_POR_SHIFT	0
#define SYSREG_USB3_POR_MASK	0x03
#define SYSREG_USB3_POR_VAL	0x3
#define SYSREG_USB2_UTMI_SHIFT	0
#define SYSREG_USB3_AHB_SHIFT	0

enum nx_phy_id {
	NX_DEVICE,
	NX_HOST,
	NX_HSIC,
	NX_NUM_PHYS,
};

static int nx_device_power_on(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 val;

	/* set POR - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_POR_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	val = readl(drv->reg_phy + SYSREG_USB3);
	/*val &= ~(SYSREG_USB3_POR_MASK);*/
	val |= SYSREG_USB3_POR_VAL << SYSREG_USB3_POR_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB3);
	/* wait 40 micro seconds */
	udelay(40);
	/* set utmi reset 1'b1 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_UTMI_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	/* set ahb reset 1'bl */
	val = readl(drv->reg_phy + SYSREG_USB3);
	val |= 1 << SYSREG_USB3_AHB_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB3);
	return 0;
}

static int nx_device_power_off(struct samsung_usb2_phy_instance *inst)
{
	return 0;
}

static const struct samsung_usb2_common_phy nx_phys[] = {
	{
		.label		= "device",
		.id		= NX_DEVICE,
		.power_on	= nx_device_power_on,
		.power_off	= nx_device_power_off,
	},
};

const struct samsung_usb2_phy_config nexell_usb2_phy_config = {
	.num_phys		= NX_NUM_PHYS,
	.phys			= nx_phys,
};
