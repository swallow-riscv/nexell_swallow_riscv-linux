/*
 * Copyright (C) 2018  Nexell Co., Ltd.
 * Author: Youngbok, Park <ybpark@nexell.co.kr>
 * Author: Seoji, Kim <seoji@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/sched_clock.h>

#define CLK_SOURCE_HZ (10 * 1000000) /* or 1MHZ */
#define CLK_EVENT_HZ (10 * 1000000)  /* or 1MHZ */

/* timer register */
#define REG_TCFG0	(0x00)
#define REG_TCFG1	(0x04)
#define REG_TCON	(0x08)
#define REG_TCNTB0	(0x0C)
#define REG_TCMPB0	(0x10)
#define REG_TCNT0	(0x14)
#define REG_CSTAT	(0x18)

#define TCON_BIT_AUTO (1 << 3)
#define TCON_BIT_INVT (1 << 2)
#define TCON_BIT_UP (1 << 1)
#define TCON_BIT_RUN (1 << 0)
#define CSTAT_IRQSTAT (1 << 5)
#define CSTAT_IRQ_EN_BIT (1 << 0)

#define timer_debug 0

/* timer data structs */
struct timer_info {
	void __iomem *base;
	int channel;
	int interrupt;
	const char *clock_name;
	struct clk *clk;
	unsigned long request;
	unsigned long rate;
	int tmux;
	int prescale;
	unsigned int tcount;
	unsigned int rcount;
};

struct timer_of_dev {
	struct timer_info timer_source;
	struct timer_info timer_event;
};

static struct timer_of_dev *timer_dev;
#define get_timer_dev() ((struct timer_of_dev *)timer_dev)

static inline void timer_clock(void __iomem *base, int mux, int scl)
{
	if (timer_debug)
		printk("[timer] timer_clock\n");
	writel(scl - 1, base + REG_TCFG0);
	writel(mux, base + REG_TCFG1);
}

static inline void timer_count(void __iomem *base, unsigned int cnt)
{
	if (timer_debug)
		printk("[timer] timrer_count\n");
	writel(cnt, base + REG_TCNTB0);
	writel(cnt, base + REG_TCMPB0);
}

static inline void timer_start(void __iomem *base, int irqon)
{
	int on = irqon ? 1 : 0;
	u32 val;
	if (timer_debug)
		printk("[timer] timer_start\n");

	writel(on, base + REG_CSTAT);
	writel(TCON_BIT_UP, base + REG_TCON);

	val = (TCON_BIT_AUTO | TCON_BIT_RUN);
	writel(val, base + REG_TCON);
}

static inline void timer_stop(void __iomem *base, int irqon)
{
	int on = irqon ? 1 : 0;
	if (timer_debug)
		printk("[timer] timer_stop\n");

	on |= CSTAT_IRQSTAT;
	writel(on, base + REG_CSTAT);
	writel(0, base + REG_TCON);
}

static inline unsigned int timer_read(void __iomem *base)
{
	if (timer_debug)
		printk("[timer] timer_read\n");
	return readl(base + REG_TCNT0);
}

static inline u32 timer_read_count(void)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_source;
	if (timer_debug)
		printk("[timer] timer_read_count\n");

	if (NULL == dev || NULL == info->base)
		return 0;

	info = &dev->timer_source;

	info->rcount = (info->tcount - timer_read(info->base));
	return (u32)info->rcount;
}

/*
 * Timer clock source
 */
static void timer_clock_select(struct timer_of_dev *dev,
			       struct timer_info *info)
{
	unsigned long rate, tout = 0;
	unsigned long mout, thz, delt = (-1UL);
	unsigned long frequency = info->request;
	int tscl = 0, tmux = 5, smux = 0, pscl = 0;
	if (timer_debug)
		printk("[timer] timer_clock_select\n");

	if (info->request) {
		clk_set_rate(info->clk, info->request);
	} else {
		pr_err("request frequency is zero\n");
		return;
	}
	clk_prepare_enable(info->clk);
	rate = clk_get_rate(info->clk);

	for (smux = 0; smux < 5; smux++) {
		mout = rate / (1 << smux);
		pscl = mout / frequency;
		thz = mout / (pscl ? pscl : 1);
		if (!(mout % frequency) && 256 > pscl) {
			tout = thz, tmux = smux, tscl = pscl;
			break;
		}
		if (pscl > 256)
			continue;
		if (abs(frequency - thz) >= delt)
			continue;
		tout = thz, tmux = smux, tscl = pscl;
		delt = abs(frequency - thz);
	}

	info->tmux = tmux;
	info->prescale = tscl;
	info->tcount = tout / HZ;
	info->rate = tout;

	pr_info("%s (mux=%d, scl=%d, rate=%ld, tcount = %d)\n",
			__func__, tmux, tscl, tout, info->tcount);
}

static void timer_source_suspend(struct clocksource *cs)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_source;
	void __iomem *base = info->base;
	if (timer_debug)
		printk("[timer] timer_source_suspend\n");

	info->rcount = (info->tcount - timer_read(base));
	timer_stop(base, 0);

	if (info->clk)
		clk_disable_unprepare(info->clk);
}

static void timer_source_resume(struct clocksource *cs)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_source;
	void __iomem *base = info->base;
	ulong flags;
	if (timer_debug)
		printk("[timer] timer_source_resume\n");

	pr_debug("%s (mux:%d, scale:%d cnt:0x%x,0x%x)\n", __func__,
		 info->tmux, info->prescale, info->rcount, info->tcount);

	local_irq_save(flags);

	if (info->clk) {
		clk_set_rate(info->clk, info->rate);
		clk_prepare_enable(info->clk);
	}

	timer_stop(base, 0);
	timer_clock(base, info->tmux, info->prescale);
	timer_count(base, info->rcount); /* restore count */
	timer_start(base, 0);
	timer_count(base, info->tcount); /* next count */

	local_irq_restore(flags);
}

static u64 timer_source_read(struct clocksource *cs)
{
	if (timer_debug)
		printk("[timer] timer_source_read\n");
	return (u64)timer_read_count();
}

static struct clocksource timer_clocksource = {
	.name = "source timer",
	.rating = 300,
	.read = timer_source_read,
	.mask = CLOCKSOURCE_MASK(32),
	.shift = 20,
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
	.suspend = timer_source_suspend,
	.resume = timer_source_resume,
};

static u64 notrace timer_source_sched_read(void)
{
	return get_cycles(); 
}
static int __init timer_source_of_init(struct device_node *node)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_source;
	struct clocksource *cs = &timer_clocksource;
	void __iomem *base = info->base;
	if (timer_debug)
		printk("[timer] timer_source_of_init\n");

	info->request = CLK_SOURCE_HZ;

	timer_clock_select(dev, info);

	/* reset tcount */
	info->tcount = 0xFFFFFFFF;

	if (timer_debug)
		printk("[timer] clocksource_register_hz\n");
	clocksource_register_hz(cs, info->rate);
	sched_clock_register(timer_source_sched_read, 64, riscv_timebase);

	timer_stop(base, 0);
	timer_clock(base, info->tmux, info->prescale);
	timer_count(base, info->tcount);
	timer_start(base, 0);

	pr_info("-timer: source, %9lu(HZ:%d), mult:%u\n", info->rate, HZ,
	       cs->mult);
	return 0;
}

/*
 * Timer clock event
 */
static void timer_event_resume(struct clock_event_device *evt)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;

	pr_debug("%s (mux:%d, scale:%d)\n", __func__, info->tmux,
		 info->prescale);

	timer_stop(base, 1);
	timer_clock(base, info->tmux, info->prescale);
}

static int timer_event_shutdown(struct clock_event_device *evt)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;
	
	if (timer_debug)
		printk("timer_event_shutdown\n");
	timer_stop(base, 0);

	return 0;
}

static int timer_event_set_oneshot(struct clock_event_device *evt)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;
	unsigned int tmp;

	if (timer_debug)
		printk("timer_event_set_oneshot\n");

	timer_stop(base, 0);
	timer_count(base, ~0);

	tmp = readl(base + REG_CSTAT);
	tmp |= CSTAT_IRQ_EN_BIT;
	tmp |= CSTAT_IRQSTAT;
	writel(tmp, base + REG_CSTAT);

	return 0;
}

static int timer_event_set_periodic(struct clock_event_device *evt)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;
	unsigned long cnt = info->tcount;
	if (timer_debug)
		printk("timer_event_set_periodic\n");

	timer_stop(base, 0);
	timer_count(base, (cnt-1));
	timer_start(base, 1);

	return 0;
}

static int timer_event_set_next(unsigned long delta,
				struct clock_event_device *evt)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;
	ulong flags;
	if (timer_debug)
		printk("timer_event_set_next\n");

	raw_local_irq_save(flags);

	timer_stop(base, 0);
	timer_count(base, (delta-1));
	timer_start(base, 1);

	raw_local_irq_restore(flags);
	return 0;
}

static struct clock_event_device timer_clock_event = {
	.name = "event timer",
	.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_state_shutdown = timer_event_shutdown,
	.set_state_periodic = timer_event_set_periodic,
	.set_state_oneshot = timer_event_set_oneshot,
	.tick_resume = timer_event_shutdown,
	.set_next_event = timer_event_set_next,
	.resume = timer_event_resume,
	.rating = 50, /* Lower than dummy timer (for 6818) */
};

static irqreturn_t timer_event_handler(int irq, void *dev_id)
{
	struct clock_event_device *evt = &timer_clock_event;
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	void __iomem *base = info->base;
	if(timer_debug)
		printk("[timer] timer_event_handler\n");

	/* clear status */
	writel(CSTAT_IRQSTAT | 0x1, base + REG_CSTAT);
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction timer_event_irqaction = {
	.name = "Event Timer IRQ",
	.flags = IRQF_TIMER, /* removed IRQF_DISABLED kernel 4.1.15 */
	.handler = timer_event_handler,
};

static int __init timer_event_of_init(struct device_node *node)
{
	struct timer_of_dev *dev = get_timer_dev();
	struct timer_info *info = &dev->timer_event;
	struct clock_event_device *evt = &timer_clock_event;
	void __iomem *base = info->base;
	int ret;

	if (timer_debug)
		printk("[timer] timer_event_of_init\n");

	info->request = CLK_EVENT_HZ;

	timer_clock_select(dev, info);
	timer_stop(base, 1);
	timer_clock(base, info->tmux, info->prescale);

	ret = setup_irq(info->interrupt, &timer_event_irqaction);
	if (ret) {
		pr_err("failed to setup irq for clockevent\n");
		return ret;
	}
	clockevents_calc_mult_shift(evt, info->rate, 5);
	evt->max_delta_ns = clockevent_delta2ns(0xffffffff, evt);
	evt->min_delta_ns = clockevent_delta2ns(0xf, evt);
	evt->cpumask = cpumask_of(0);
	evt->irq = info->interrupt;

	if (timer_debug)
		printk("[timer] clockevents_register_device\n");
	clockevents_register_device(evt);

	pr_info("-timer: event , %9lu(HZ:%d), mult:%u\n",
			info->rate, HZ, evt->mult);
	return 0;
}

static int __init
timer_get_device_data(struct device_node *node, struct timer_of_dev *dev)
{
	struct timer_info *tsrc = &dev->timer_source;
	struct timer_info *tevt = &dev->timer_event;
	void __iomem *base;
	if (timer_debug)
		printk("[timer] timer_get_device_data\n");

	base = of_iomap(node, 0);

	if (IS_ERR(base)) {
		pr_err("Can't map registers for timer!");
		return -EINVAL;
	}

	if (of_property_read_u32(node, "clksource", &tsrc->channel)) {
		pr_err("timer node is missing 'clksource'\n");
		return -EINVAL;
	}

	if (of_property_read_u32(node, "clkevent", &tevt->channel)) {
		pr_err("timer node is missing 'clkevent'\n");
		return -EINVAL;
	}
	if (tsrc->channel == tevt->channel) {
		pr_err(" 'clksource' and 'clkevent' is same channel\n");
		return -EINVAL;
	}

	tsrc->base = base + tsrc->channel * 0x100;
	tevt->base = base + tevt->channel * 0x100;

	tevt->interrupt = irq_of_parse_and_map(node, 0);

	tsrc->clk = of_clk_get(node, 0);
	if (IS_ERR(tsrc->clk)) {
		pr_err("failed timer tsrc clock\n");
		return -EINVAL;
	}

	tevt->clk = of_clk_get(node, 1);
	if (IS_ERR(tevt->clk)) {
		pr_err("failed timer event clock\n");
		return -EINVAL;
	}

	pr_debug("%s : irq %d\n", node->name, tevt->interrupt);

	return 0;
}

static struct delay_timer nexell_delay_timer = {
	.freq = CLK_SOURCE_HZ,
	.read_current_timer = (unsigned long (*)(void))timer_read_count,
};

static int __init timer_of_init_dt(struct device_node *node)
{
	struct timer_of_dev *dev;
	if (timer_debug)
		printk("[timer] timer_of_init_dt\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	timer_dev = dev;

	if (timer_get_device_data(node, dev))
		panic("unable to map timer cpu\n");

	/* source timer initialize */
	timer_source_of_init(node);
	/* event timer initialize */
	timer_event_of_init(node);
	/* delay timer initialze */
	register_current_timer_delay(&nexell_delay_timer);

	return 0;
}
/* initialize */
CLOCKSOURCE_OF_DECLARE(sip_n31nx, "nexell,nexell-timer", timer_of_init_dt);
