/*
 * Copyright (c) 2018 Nexell Co., Ltd.
 * Author: Choonghyun Jeon <suker@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file includes utility functions to register clocks to common
 * clock framework for Nexell platforms.
*/

#include <linux/slab.h>
#include <linux/clkdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>

#include "clk.h"

static LIST_HEAD(clock_reg_cache_list);

void nexell_clk_save(void __iomem *base,
				    struct nexell_clk_reg_dump *rd,
				    unsigned int num_regs)
{
	for (; num_regs > 0; --num_regs, ++rd)
		rd->value = readl(base + rd->offset);
}

void nexell_clk_restore(void __iomem *base,
				      const struct nexell_clk_reg_dump *rd,
				      unsigned int num_regs)
{
	for (; num_regs > 0; --num_regs, ++rd)
		writel(rd->value, base + rd->offset);
}

struct nexell_clk_reg_dump *nexell_clk_alloc_reg_dump(
						const unsigned long *rdump,
						unsigned long nr_rdump)
{
	struct nexell_clk_reg_dump *rd;
	unsigned int i;

	rd = kcalloc(nr_rdump, sizeof(*rd), GFP_KERNEL);
	if (!rd)
		return NULL;

	for (i = 0; i < nr_rdump; ++i)
		rd[i].offset = rdump[i];

	return rd;
}

/* setup the essentials required to support clock lookup using ccf */
struct nexell_clk_provider *__init nexell_clk_init(struct device_node *np,
			void __iomem *base, unsigned long nr_clks)
{
	struct nexell_clk_provider *ctx;
	int i;

	ctx = kzalloc(sizeof(struct nexell_clk_provider) +
		      sizeof(*ctx->clk_data.hws) * nr_clks, GFP_KERNEL);
	if (!ctx)
		panic("could not allocate clock provider context.\n");

	for (i = 0; i < nr_clks; ++i)
		ctx->clk_data.hws[i] = ERR_PTR(-ENOENT);

	ctx->reg_base = base;
	ctx->clk_data.num = nr_clks;
	spin_lock_init(&ctx->lock);

	return ctx;
}

void __init nexell_clk_of_add_provider(struct device_node *np,
				struct nexell_clk_provider *ctx)
{
	if (np) {
		if (of_clk_add_hw_provider(np, of_clk_hw_onecell_get,
					&ctx->clk_data))
			panic("could not register clk provider\n");
	}
}

/* add a clock instance to the clock lookup table used for dt based lookup */
void nexell_clk_add_lookup(struct nexell_clk_provider *ctx,
			    struct clk_hw *clk_hw, unsigned int id)
{
	if (id)
		ctx->clk_data.hws[id] = clk_hw;
}

/* register a list of fixed clocks */
void __init nexell_clk_register_fixed_rate(struct nexell_clk_provider *ctx,
		const struct nexell_fixed_rate_clock *list,
		unsigned int nr_clk)
{
	struct clk_hw *clk_hw;
	unsigned int idx, ret;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		clk_hw = clk_hw_register_fixed_rate(ctx->dev, list->name,
			list->parent_name, list->flags, list->fixed_rate);
		if (IS_ERR(clk_hw)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				list->name);
			continue;
		}

		nexell_clk_add_lookup(ctx, clk_hw, list->id);

		/*
		 * Unconditionally add a clock lookup for the fixed rate clocks.
		 * There are not many of these on any of Nexell platforms.
		 */
		ret = clk_hw_register_clkdev(clk_hw, list->name, NULL);
		if (ret)
			pr_err("%s: failed to register clock lookup for %s",
				__func__, list->name);
	}
}

/* register a list of fixed factor clocks */
void __init nexell_clk_register_fixed_factor(struct nexell_clk_provider *ctx,
		const struct nexell_fixed_factor_clock *list, unsigned int nr_clk)
{
	struct clk_hw *clk_hw;
	unsigned int idx;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		clk_hw = clk_hw_register_fixed_factor(ctx->dev, list->name,
			list->parent_name, list->flags, list->mult, list->div);
		if (IS_ERR(clk_hw)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				list->name);
			continue;
		}

		nexell_clk_add_lookup(ctx, clk_hw, list->id);
	}
}

/* register a list of gate clocks */
void __init nexell_clk_register_gate(struct nexell_clk_provider *ctx,
				const struct nexell_gate_clock *list,
				unsigned int nr_clk)
{
	struct clk_hw *clk_hw;
	unsigned int idx;

	for (idx = 0; idx < nr_clk; idx++, list++) {
		clk_hw = clk_hw_register_gate(ctx->dev, list->name, list->parent_name,
				list->flags, ctx->reg_base + list->offset,
				list->bit_idx, list->gate_flags, &ctx->lock);
		if (IS_ERR(clk_hw)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				list->name);
			continue;
		}

		nexell_clk_add_lookup(ctx, clk_hw, list->id);
	}
}

/*
 * obtain the clock speed of all external fixed clock sources from device
 * tree and register it
 */
void __init nexell_clk_of_register_fixed_ext(struct nexell_clk_provider *ctx,
			struct nexell_fixed_rate_clock *fixed_rate_clk,
			unsigned int nr_fixed_rate_clk,
			const struct of_device_id *clk_matches)
{
	const struct of_device_id *match;
	struct device_node *clk_np;
	u32 freq;

	for_each_matching_node_and_match(clk_np, clk_matches, &match) {
		if (of_property_read_u32(clk_np, "clock-frequency", &freq))
			continue;
		fixed_rate_clk[(unsigned long)match->data].fixed_rate = freq;
	}
	nexell_clk_register_fixed_rate(ctx, fixed_rate_clk, nr_fixed_rate_clk);
}

/* utility function to get the rate of a specified clock */
unsigned long _get_rate(const char *clk_name)
{
	struct clk *clk;

	clk = __clk_lookup(clk_name);
	if (!clk) {
		pr_err("%s: could not find clock %s\n", __func__, clk_name);
		return 0;
	}

	return clk_get_rate(clk);
}

#ifdef CONFIG_PM_SLEEP
static int nexell_clk_suspend(void)
{
	struct nexell_clock_reg_cache *reg_cache;

	list_for_each_entry(reg_cache, &clock_reg_cache_list, node)
		nexell_clk_save(reg_cache->reg_base, reg_cache->rdump,
				reg_cache->rd_num);
	return 0;
}

static void nexell_clk_resume(void)
{
	struct nexell_clock_reg_cache *reg_cache;

	list_for_each_entry(reg_cache, &clock_reg_cache_list, node)
		nexell_clk_restore(reg_cache->reg_base, reg_cache->rdump,
				reg_cache->rd_num);
}

static struct syscore_ops nexell_clk_syscore_ops = {
	.suspend = nexell_clk_suspend,
	.resume = nexell_clk_resume,
};

void nexell_clk_sleep_init(void __iomem *reg_base,
			const unsigned long *rdump,
			unsigned long nr_rdump)
{
	struct nexell_clock_reg_cache *reg_cache;

	reg_cache = kzalloc(sizeof(struct nexell_clock_reg_cache),
			GFP_KERNEL);
	if (!reg_cache)
		panic("could not allocate register reg_cache.\n");
	reg_cache->rdump = nexell_clk_alloc_reg_dump(rdump, nr_rdump);

	if (!reg_cache->rdump)
		panic("could not allocate register dump storage.\n");

	if (list_empty(&clock_reg_cache_list))
		register_syscore_ops(&nexell_clk_syscore_ops);

	reg_cache->reg_base = reg_base;
	reg_cache->rd_num = nr_rdump;
	list_add_tail(&reg_cache->node, &clock_reg_cache_list);
}

#else
void nexell_clk_sleep_init(void __iomem *reg_base,
			const unsigned long *rdump,
			unsigned long nr_rdump) {}
#endif

/*
 * Common function which registers plls, muxes, dividers and gates
 * for each CMU. It also add CMU register list to register cache.
 */
struct nexell_clk_provider * __init nexell_cmu_register_one(
			struct device_node *np,
			const struct nexell_cmu_info *cmu)
{
	void __iomem *reg_base;
	struct nexell_clk_provider *ctx;

	reg_base = of_iomap(np, 0);
	if (!reg_base) {
		panic("%s: failed to map registers\n", __func__);
		return NULL;
	}

	ctx = nexell_clk_init(np, reg_base, cmu->nr_clk_ids);
	if (!ctx) {
		panic("%s: unable to allocate ctx\n", __func__);
		return ctx;
	}

	if (cmu->gate_clks)
		nexell_clk_register_gate(ctx, cmu->gate_clks,
			cmu->nr_gate_clks);
	if (cmu->fixed_clks)
		nexell_clk_register_fixed_rate(ctx, cmu->fixed_clks,
			cmu->nr_fixed_clks);
	if (cmu->fixed_factor_clks)
		nexell_clk_register_fixed_factor(ctx, cmu->fixed_factor_clks,
			cmu->nr_fixed_factor_clks);
	if (cmu->clk_regs)
		nexell_clk_sleep_init(reg_base, cmu->clk_regs,
			cmu->nr_clk_regs);

	nexell_clk_of_add_provider(np, ctx);

	return ctx;
}
