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

#define SYSREG_USB0			0x90
#define SYSREG_USB1			0x94
#define SYSREG_USB2			0x98
#define SYSREG_USB3			0x128

#define SYSREG_USB2_PORENB_SHIFT	3
#define SYSREG_USB2_POR_SHIFT		2
#define SYSREG_USB2_UTMI_SHIFT		0
#define SYSREG_USB2_AHB_SHIFT		2

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

	/* clear POR - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val &= ~(1 << SYSREG_USB2_POR_SHIFT);
	writel(val, drv->reg_phy + SYSREG_USB2);

	/* set PORENB - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_PORENB_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	udelay(1);

	/* set POR - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_POR_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);

	/* set PORENB - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_PORENB_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	udelay(1);

	/* clear POR - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val &= ~(1 << SYSREG_USB2_POR_SHIFT);
	writel(val, drv->reg_phy + SYSREG_USB2);

	/* wait 10 micro seconds */
	udelay(10);

	/* set utmi reset 1'b1 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_UTMI_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	udelay(1);

	/* set ahb reset 1'bl */
	val = readl(drv->reg_phy + SYSREG_USB3);
	val |= 1 << SYSREG_USB2_AHB_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB3);
	udelay(1);

	return 0;
}

static int nx_device_power_off(struct samsung_usb2_phy_instance *inst)
{
	struct samsung_usb2_phy_driver *drv = inst->drv;
	u32 val;

	/* clear ahb reset 1'b0 */
	val = readl(drv->reg_phy + SYSREG_USB3);
	val &= ~(1 << SYSREG_USB2_AHB_SHIFT);
	writel(val, drv->reg_phy + SYSREG_USB3);
	udelay(10);

	/* clear utmi reset 1'b0 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val &= ~(1 << SYSREG_USB2_UTMI_SHIFT);
	writel(val, drv->reg_phy + SYSREG_USB2);
	udelay(10);

	/* set POR - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_POR_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);

	/* set PORENB - 2'b10 */
	val = readl(drv->reg_phy + SYSREG_USB2);
	val |= 1 << SYSREG_USB2_PORENB_SHIFT;
	writel(val, drv->reg_phy + SYSREG_USB2);
	udelay(10);

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
