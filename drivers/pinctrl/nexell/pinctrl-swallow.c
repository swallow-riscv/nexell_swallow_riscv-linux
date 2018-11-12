/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) 2018  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/err.h>

#include "pinctrl-nexell.h"
#include "pinctrl-swallow.h"

static struct {
	struct nx_gpio_reg_set *gpio_regs;
	struct nx_gpio_reg_set gpio_save;
} gpio_modules[NR_GPIO_MODULE];

/*
 * gpio functions
 */

static void nx_gpio_setbit(u32 *p, u32 bit, bool enable)
{
	u32 newvalue = readl(p);

	newvalue &= ~(1UL << bit);
	newvalue |= (u32)enable << bit;

	writel(newvalue, p);
}

static bool nx_gpio_getbit(u32 value, u32 bit)
{
	return (bool)((value >> bit) & (1UL));
}

static void nx_gpio_setbit2(u32 *p, u32 bit, u32 value)
{
	u32 newvalue = readl(p);

	newvalue = (u32)(newvalue & ~(3UL << (bit * 2)));
	newvalue = (u32)(newvalue | (value << (bit * 2)));

	writel(newvalue, p);
}

static u32 nx_gpio_getbit2(u32 value, u32 bit)
{
	return (u32)((u32)(value >> (bit * 2)) & 3UL);
}

static bool nx_gpio_open_module(u32 idx)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	writel(0xFFFFFFFF, &regs->gpio_slew_disable_default);
	writel(0xFFFFFFFF, &regs->gpio_drv1_disable_default);
	writel(0xFFFFFFFF, &regs->gpio_drv0_disable_default);
	writel(0xFFFFFFFF, &regs->gpio_pullsel_disable_default);
	writel(0xFFFFFFFF, &regs->gpio_pullenb_disable_default);
	writel(0xFFFFFFFF, &regs->gpio_inenb_disable_default);

	return true;
}

static void nx_gpio_set_output_enable(u32 idx, u32 bitnum, bool enable)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	if (enable)
		nx_gpio_setbit(&regs->gpio_outenb, bitnum, 0);
	else
		nx_gpio_setbit(&regs->gpio_outenb, bitnum, 1);
}

static bool nx_gpio_get_output_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&regs->gpio_outenb), bitnum);
}

static void nx_gpio_set_output_value(u32 idx, u32 bitnum, bool value)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&regs->gpio_out, bitnum, value);
}

static bool nx_gpio_get_input_value(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&regs->gpio_pad), bitnum);
}

static void nx_gpio_set_pad_function(u32 idx, u32 bitnum, int fn)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&regs->gpio_altfn[0], bitnum, (u32)fn);
}

static int nx_gpio_get_pad_function(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	return nx_gpio_getbit(readl(&regs->gpio_altfn[0]), bitnum);
}

static void nx_gpio_set_drive_strength(u32 idx, u32 bitnum, int drvstrength)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	nx_gpio_setbit(&regs->gpio_drv1, bitnum,
		       (bool)(((u32)drvstrength >> 0) & 0x1));
	nx_gpio_setbit(&regs->gpio_drv0, bitnum,
		       (bool)(((u32)drvstrength >> 1) & 0x1));
}

static int nx_gpio_get_drive_strength(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *regs;
	u32 value;

	regs = gpio_modules[idx].gpio_regs;

	value = nx_gpio_getbit(readl(&regs->gpio_drv0), bitnum) << 1;
	value |= nx_gpio_getbit(readl(&regs->gpio_drv1), bitnum) << 0;

	return (int)value;
}

static void nx_gpio_set_pull_enable(u32 idx, u32 bitnum, int pullsel)
{
	struct nx_gpio_reg_set *regs;

	regs = gpio_modules[idx].gpio_regs;

	if (pullsel == nx_gpio_pull_down || pullsel == nx_gpio_pull_up) {
		nx_gpio_setbit(&regs->gpio_pullsel, bitnum, (bool)pullsel);
		nx_gpio_setbit(&regs->gpio_pullenb, bitnum, true);
	} else
		nx_gpio_setbit(&regs->gpio_pullenb, bitnum, false);
}

static int nx_gpio_get_pull_enable(u32 idx, u32 bitnum)
{
	struct nx_gpio_reg_set *regs;
	bool enable;

	regs = gpio_modules[idx].gpio_regs;
	enable = nx_gpio_getbit(readl(&regs->gpio_pullenb), bitnum);

	if (enable == true)
		return (int)nx_gpio_getbit(readl(&regs->gpio_pullsel), bitnum);
	else
		return (nx_gpio_pull_off);
}

/*
 * GPIO Operation.
 */

const unsigned char (*gpio_fn_no)[GPIO_NUM_PER_BANK] = NULL;

const unsigned char swallow_pio_fn_no[][GPIO_NUM_PER_BANK] = {
	ALT_NO_GPIO_A, ALT_NO_GPIO_B, ALT_NO_GPIO_C,
	ALT_NO_GPIO_D, ALT_NO_GPIO_E, ALT_NO_GPIO_F,
	ALT_NO_GPIO_G,
};

/*----------------------------------------------------------------------------*/
static spinlock_t lock[NR_GPIO_MODULE + 1]; /* A, B, C, D, E, F, G */
static unsigned long lock_flags[NR_GPIO_MODULE + 1];

#define IO_LOCK_INIT(x) spin_lock_init(&lock[x])
#define IO_LOCK(x) spin_lock_irqsave(&lock[x], lock_flags[x])
#define IO_UNLOCK(x) spin_unlock_irqrestore(&lock[x], lock_flags[x])

void nx_soc_gpio_set_io_func(unsigned int io, unsigned int func)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		nx_gpio_set_pad_function(grp, bit, func);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};
}

int nx_soc_gpio_get_altnum(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	return gpio_fn_no[grp][bit];
}

unsigned int nx_soc_gpio_get_io_func(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	unsigned int fn = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		fn = nx_gpio_get_pad_function(grp, bit);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};

	return fn;
}

void nx_soc_gpio_set_io_dir(unsigned int io, int out)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		nx_gpio_set_output_enable(grp, bit, out ? true : false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};
}

int nx_soc_gpio_get_io_dir(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int dir = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		dir = nx_gpio_get_output_enable(grp, bit) ? 0 : 1;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};

	return dir;
}

void nx_soc_gpio_set_io_pull(unsigned int io, int val)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d) sel:%d\n", __func__, grp, bit, val);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		nx_gpio_set_pull_enable(grp, bit, val);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};
}

int nx_soc_gpio_get_io_pull(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int up = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		up = nx_gpio_get_pull_enable(grp, bit);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};

	return up;
}

void nx_soc_gpio_set_io_drv(int io, int drv)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d) drv:%d\n", __func__, grp, bit, drv);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		nx_gpio_set_drive_strength(grp, bit, drv);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};
}

int nx_soc_gpio_get_io_drv(int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int drv = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		drv = nx_gpio_get_drive_strength(grp, bit);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};

	return drv;
}

void nx_soc_gpio_set_out_value(unsigned int io, int high)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		nx_gpio_set_output_value(grp, bit, high ? true : false);
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};
}

int nx_soc_gpio_get_in_value(unsigned int io)
{
	unsigned int grp = PAD_GET_GROUP(io);
	unsigned int bit = PAD_GET_BITNO(io);
	int val = -1;

	pr_debug("%s (%d.%02d)\n", __func__, grp, bit);

	switch (io & ~(32 - 1)) {
	case PAD_GPIO_A:
	case PAD_GPIO_B:
	case PAD_GPIO_C:
	case PAD_GPIO_D:
	case PAD_GPIO_E:
	case PAD_GPIO_F:
	case PAD_GPIO_G:
	case PAD_GPIO_H:
		IO_LOCK(grp);
		val = nx_gpio_get_input_value(grp, bit) ? 1 : 0;
		IO_UNLOCK(grp);
		break;
	default:
		pr_err("fail gpio io:%d, group:%d (%s)\n", io, grp, __func__);
		break;
	};

	return val;
}

int nx_soc_is_gpio_pin(unsigned int io)
{
	return 1;
}

static int swallow_gpio_device_init(struct list_head *banks, int nr_banks)
{
	struct module_init_data *init_data;
	int i;

	for (i = 0; i < NR_GPIO_MODULE + 1; i++)
		IO_LOCK_INIT(i);

	gpio_fn_no = swallow_pio_fn_no;

	i = 0;
	list_for_each_entry(init_data, banks, node) {
		if (init_data->bank_type == EINT_TYPE_GPIO) {
			gpio_modules[i].gpio_regs =
			    (struct nx_gpio_reg_set *)(init_data->bank_base);

			nx_gpio_open_module(i);
			i++;
		}
	}

	return 0;
}

/*
 * irq_chip functions
 */

static void irq_gpio_ack(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	writel((1 << bit), base + GPIO_INT_STATUS); /* irq pend clear */
}

static void irq_gpio_mask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_unmask(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
}

static int irq_gpio_set_type(struct irq_data *irqd, unsigned int type)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;
	u32 val, alt;
	ulong reg;

	int mode = 0;

	pr_debug("%s: gpio irq=%d, %s.%d, type=0x%x\n", __func__, bank->irq,
		 bank->name, bit, type);

	switch (type) {
	case IRQ_TYPE_NONE:
		pr_warn("%s: No edge setting!\n", __func__);
		break;
	case IRQ_TYPE_EDGE_RISING:
		mode = NX_GPIO_INTMODE_RISINGEDGE;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = NX_GPIO_INTMODE_FALLINGEDGE;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = NX_GPIO_INTMODE_BOTHEDGE;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode = NX_GPIO_INTMODE_LOWLEVEL;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = NX_GPIO_INTMODE_HIGHLEVEL;
		break;
	default:
		pr_err("%s: No such irq type %d", __func__, type);
		return -1;
	}

	/*
	 * must change mode to gpio to use gpio interrupt
	 */

	/* gpio input : output disable, input enable */
	writel(readl(base + GPIO_OUT_ENB) & ~(1 << bit), base + GPIO_OUT_ENB);
	writel(readl(base + GPIO_IN_ENB) | (1 << bit), base + GPIO_IN_ENB);

	/* gpio mode : interrupt mode */
	reg = (ulong)(base + GPIO_INT_MODE + (bit / 16) * 4);
	val = (readl((void *)reg) & ~(3 << ((bit & 0xf) * 2))) |
	      ((mode & 0x3) << ((bit & 0xf) * 2));
	writel(val, (void *)reg);

	reg = (ulong)(base + GPIO_INT_MODEEX);
	val = (readl((void *)reg) & ~(1 << bit)) | (((mode >> 2) & 0x1) << bit);
	writel(val, (void *)reg);

	/* gpio alt : gpio mode for irq */
	reg = (ulong)(base + GPIO_ALTFN + (bit / 16) * 4);
	val = readl((void *)reg) & ~(3 << ((bit & 0xf) * 2));
	alt = nx_soc_gpio_get_altnum(bank->grange.pin_base + bit);
	val |= alt << ((bit & 0xf) * 2);
	writel(val, (void *)reg);
	writel(readl(base + GPIO_ALTFNEX) & ~(1 << bit), base + GPIO_ALTFNEX);
	pr_debug("%s: set func to gpio. alt:%d, base:%d, bit:%d\n", __func__,
		 alt, bank->grange.pin_base, bit);

	return 0;
}

static void irq_gpio_enable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* unmask:irq enable */
	writel(readl(base + GPIO_INT_ENB) | (1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) | (1 << bit), base + GPIO_INT_DET);
}

static void irq_gpio_disable(struct irq_data *irqd)
{
	struct nexell_pin_bank *bank = irq_data_get_irq_chip_data(irqd);
	int bit = (int)(irqd->hwirq);
	void __iomem *base = bank->virt_base;

	pr_debug("%s: gpio irq=%d, %s.%d\n", __func__, bank->irq, bank->name,
		 bit);

	/* mask:irq disable */
	writel(readl(base + GPIO_INT_ENB) & ~(1 << bit), base + GPIO_INT_ENB);
	writel(readl(base + GPIO_INT_DET) & ~(1 << bit), base + GPIO_INT_DET);
}

/*
 * irq_chip for gpio interrupts.
 */
static struct irq_chip swallow_gpio_irq_chip = {
	.name = "GPIO",
	.irq_ack = irq_gpio_ack,
	.irq_mask = irq_gpio_mask,
	.irq_unmask = irq_gpio_unmask,
	.irq_set_type = irq_gpio_set_type,
	.irq_enable = irq_gpio_enable,
	.irq_disable = irq_gpio_disable,
	.flags = IRQCHIP_SKIP_SET_WAKE,
};

static int swallow_gpio_irq_map(struct irq_domain *h, unsigned int virq,
				irq_hw_number_t hw)
{
	struct nexell_pin_bank *b = h->host_data;

	pr_debug("%s domain map: virq %d and hw %d\n", __func__, virq, (int)hw);

	irq_set_chip_data(virq, b);
	irq_set_chip_and_handler(virq, &swallow_gpio_irq_chip,
				 handle_level_irq);
	return 0;
}

/*
 * irq domain callbacks for external gpio interrupt controller.
 */
static const struct irq_domain_ops swallow_gpio_irqd_ops = {
	.map = swallow_gpio_irq_map,
	.xlate = irq_domain_xlate_twocell,
};

static irqreturn_t swallow_gpio_irq_handler(int irq, void *data)
{
	struct nexell_pin_bank *bank = data;
	void __iomem *base = bank->virt_base;
	u32 stat, mask;
	unsigned int virq;
	int bit;

	mask = readl(base + GPIO_INT_ENB);
	stat = readl(base + GPIO_INT_STATUS) & mask;
	bit = ffs(stat) - 1;

	if (bit == -1) {
		pr_err("Unknown gpio irq=%d, status=0x%08x, mask=0x%08x\r\n",
		       irq, stat, mask);
		writel(-1, (base + GPIO_INT_STATUS)); /* clear gpio status all*/
		return IRQ_NONE;
	}

	virq = irq_linear_revmap(bank->irq_domain, bit);
	if (!virq)
		return IRQ_NONE;

	pr_debug("Gpio irq=%d [%d] (hw %u), stat=0x%08x, mask=0x%08x\n", irq,
		 bit, virq, stat, mask);
	generic_handle_irq(virq);

	return IRQ_HANDLED;
}

/*
 * swallow_gpio_irq_init() - setup handling of external gpio interrupts.
 * @d: driver data of nexell pinctrl driver.
 */
static int swallow_gpio_irq_init(struct nexell_pinctrl_drv_data *d)
{
	struct nexell_pin_bank *bank;
	struct device *dev = d->dev;
	int ret;
	int i;

	bank = d->ctrl->pin_banks;
	for (i = 0; i < d->ctrl->nr_banks; ++i, ++bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;

		ret = devm_request_irq(dev, bank->irq,
				       swallow_gpio_irq_handler,
				       IRQF_SHARED, dev_name(dev), bank);
		if (ret) {
			dev_err(dev, "irq request failed\n");
			ret = -ENXIO;
			goto err_domains;
		}

		bank->irq_domain = irq_domain_add_linear(
				bank->of_node, bank->nr_pins,
				&swallow_gpio_irqd_ops, bank);
		if (!bank->irq_domain) {
			dev_err(dev, "gpio irq domain add failed\n");
			ret = -ENXIO;
			goto err_domains;
		}
	}

	return 0;

err_domains:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (bank->eint_type != EINT_TYPE_GPIO)
			continue;
		irq_domain_remove(bank->irq_domain);
		devm_free_irq(dev, bank->irq, d);
	}

	return ret;
}

static int swallow_base_init(struct nexell_pinctrl_drv_data *drvdata)
{
	struct nexell_pin_ctrl *ctrl = drvdata->ctrl;
	int nr_banks = ctrl->nr_banks;
	int ret;
	int i;
	struct module_init_data *init_data, *n;
	LIST_HEAD(banks);

	for (i = 0; i < nr_banks; i++) {
		struct nexell_pin_bank *bank = &ctrl->pin_banks[i];

		init_data = kmalloc(sizeof(*init_data), GFP_KERNEL);
		if (!init_data) {
			ret = -ENOMEM;
			goto done;
		}

		INIT_LIST_HEAD(&init_data->node);
		init_data->bank_base = bank->virt_base;
		init_data->bank_type = bank->eint_type;

		list_add_tail(&init_data->node, &banks);
	}

	swallow_gpio_device_init(&banks, nr_banks);

done:
	/* free */
	list_for_each_entry_safe(init_data, n, &banks, node) {
		list_del(&init_data->node);
		kfree(init_data);
	}

	return 0;
}

/* pin banks of swallow pin-controller */
static struct nexell_pin_bank swallow_pin_banks[] = {
	SOC_PIN_BANK_EINTG(32, "gpioa"),
	SOC_PIN_BANK_EINTG(32, "gpiob"),
	SOC_PIN_BANK_EINTG(32, "gpioc"),
	SOC_PIN_BANK_EINTG(32, "gpiod"),
	SOC_PIN_BANK_EINTG(32, "gpioe"),
	SOC_PIN_BANK_EINTG(32, "gpiof"),
	SOC_PIN_BANK_EINTG(32, "gpiog"),
	SOC_PIN_BANK_EINTG(32, "gpioh"),
};

/*
 * Nexell pinctrl driver data for SoC.
 */
const struct nexell_pin_ctrl swallow_pin_ctrl[] = {
	{
		.pin_banks = swallow_pin_banks,
		.nr_banks = ARRAY_SIZE(swallow_pin_banks),
		.base_init = swallow_base_init,
		.gpio_irq_init = swallow_gpio_irq_init,
	},
};
