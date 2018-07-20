/*
 * Copyright (c) 2018 Nexell Co., Ltd.
 * Copyright (c) 2013 Linaro Ltd.
 * Author: Choonghyun Jeon <suker@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Common Clock Framework support for all Nexell platforms
*/

#ifndef __NEXELL_CLK_H
#define __NEXELL_CLK_H

#include <linux/clk-provider.h>

/**
 * struct nexell_clk_provider: information about clock provider
 * @reg_base: virtual address for the register base.
 * @lock: maintains exclusion between callbacks for a given clock-provider.
 * @clk_data: holds clock related data like clk_hw* and number of clocks.
 */
struct nexell_clk_provider {
	void __iomem *reg_base;
	struct device *dev;
	spinlock_t lock;
	/* clk_data must be the last entry due to variable lenght 'hws' array */
	struct clk_hw_onecell_data clk_data;
};

#define MHZ (1000 * 1000)

/**
 * struct nexell_fixed_rate_clock: information about fixed-rate clock
 * @id: platform specific id of the clock.
 * @name: name of this fixed-rate clock.
 * @parent_name: optional parent clock name.
 * @flags: optional fixed-rate clock flags.
 * @fixed-rate: fixed clock rate of this clock.
 */
struct nexell_fixed_rate_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		fixed_rate;
};

#define FRATE(_id, cname, pname, f, frate)		\
	{						\
		.id		= _id,			\
		.name		= cname,		\
		.parent_name	= pname,		\
		.flags		= f,			\
		.fixed_rate	= frate,		\
	}

/*
 * struct nexell_fixed_factor_clock: information about fixed-factor clock
 * @id: platform specific id of the clock.
 * @name: name of this fixed-factor clock.
 * @parent_name: parent clock name.
 * @mult: fixed multiplication factor.
 * @div: fixed division factor.
 * @flags: optional fixed-factor clock flags.
 */
struct nexell_fixed_factor_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		mult;
	unsigned long		div;
	unsigned long		flags;
};

#define FFACTOR(_id, cname, pname, m, d, f)		\
	{						\
		.id		= _id,			\
		.name		= cname,		\
		.parent_name	= pname,		\
		.mult		= m,			\
		.div		= d,			\
		.flags		= f,			\
	}

/**
 * struct nexell_gate_clock: information about gate clock
 * @id: platform specific id of the clock.
 * @name: name of this gate clock.
 * @parent_name: name of the parent clock.
 * @flags: optional flags for basic clock.
 * @offset: offset of the register for configuring the gate.
 * @bit_idx: bit index of the gate control bit-field in @reg.
 * @gate_flags: flags for gate-type clock.
 */
struct nexell_gate_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		offset;
	u8			bit_idx;
	u8			gate_flags;
};

#define __GATE(_id, cname, pname, o, b, f, gf)			\
	{							\
		.id		= _id,				\
		.name		= cname,			\
		.parent_name	= pname,			\
		.flags		= f,				\
		.offset		= o,				\
		.bit_idx	= b,				\
		.gate_flags	= gf,				\
	}

#define GATE(_id, cname, pname, o, b, f, gf)			\
    __GATE(_id, cname, pname, o, b, (f) | CLK_SET_RATE_PARENT, gf)

#define PNAME(x) static const char * const x[] __initconst

/**
 * struct nexell_clk_reg_dump: register dump of clock controller registers.
 * @offset: clock register offset from the controller base address.
 * @value: the value to be register at offset.
 */
struct nexell_clk_reg_dump {
	u32	offset;
	u32	value;
};


struct nexell_clock_reg_cache {
	struct list_head node;
	void __iomem *reg_base;
	struct nexell_clk_reg_dump *rdump;
	unsigned int rd_num;
};

struct nexell_cmu_info {
	/* list of pll clocks and respective count */
	/* const struct nexell_pll_clock *pll_clks; */
	/* unsigned int nr_pll_clks; */
	/* list of gate clocks and respective count */
	const struct nexell_gate_clock *gate_clks;
	unsigned int nr_gate_clks;
	/* list of fixed clocks and respective count */
	const struct nexell_fixed_rate_clock *fixed_clks;
	unsigned int nr_fixed_clks;
	/* list of fixed factor clocks and respective count */
	const struct nexell_fixed_factor_clock *fixed_factor_clks;
	unsigned int nr_fixed_factor_clks;
	/* total number of clocks with IDs assigned*/
	unsigned int nr_clk_ids;

	/* list and number of clocks registers */
	const unsigned long *clk_regs;
	unsigned int nr_clk_regs;

	/* list and number of clocks registers to set before suspend */
	const struct nexell_clk_reg_dump *suspend_regs;
	unsigned int nr_suspend_regs;
	/* name of the parent clock needed for CMU register access */
	const char *clk_name;
};

extern struct nexell_clk_provider *__init nexell_clk_init(
			struct device_node *np, void __iomem *base,
			unsigned long nr_clks);
extern void __init nexell_clk_of_add_provider(struct device_node *np,
			struct nexell_clk_provider *ctx);

extern void nexell_clk_add_lookup(struct nexell_clk_provider *ctx,
			struct clk_hw *clk_hw, unsigned int id);

extern void __init nexell_clk_register_fixed_rate(
			struct nexell_clk_provider *ctx,
			const struct nexell_fixed_rate_clock *clk_list,
			unsigned int nr_clk);
extern void __init nexell_clk_register_fixed_factor(
			struct nexell_clk_provider *ctx,
			const struct nexell_fixed_factor_clock *list,
			unsigned int nr_clk);
extern void __init nexell_clk_register_gate(struct nexell_clk_provider *ctx,
			const struct nexell_gate_clock *clk_list,
			unsigned int nr_clk);

extern struct nexell_clk_provider __init *nexell_cmu_register_one(
			struct device_node *,
			const struct nexell_cmu_info *);

extern unsigned long _get_rate(const char *clk_name);

extern void nexell_clk_sleep_init(void __iomem *reg_base,
			const unsigned long *rdump,
			unsigned long nr_rdump);

extern void nexell_clk_save(void __iomem *base,
			struct nexell_clk_reg_dump *rd,
			unsigned int num_regs);
extern void nexell_clk_restore(void __iomem *base,
			const struct nexell_clk_reg_dump *rd,
			unsigned int num_regs);
extern struct nexell_clk_reg_dump *nexell_clk_alloc_reg_dump(
			const unsigned long *rdump,
			unsigned long nr_rdump);

#endif /* __NEXELL_CLK_H */
