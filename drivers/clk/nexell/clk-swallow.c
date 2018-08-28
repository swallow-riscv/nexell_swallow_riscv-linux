/*
 * Copyright (c) 2018 Nexell Co., Ltd.
 * Author: Choonghyun Jeon <suker@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Common Clock Framework support for Swallow SoC.
*/

#include <dt-bindings/clock/swallow-clk.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include "clk.h"

#define SYS0_CLK400	        	0x0200
#define CPU0_CORE                       0x0400

static void __iomem *reg_base;

/* fixed rate clocks */
static const struct nexell_fixed_rate_clock swallow_fixed_rate_clks[] __initconst = {
	/* FRATE(0, "oscclk", NULL, 0,  25000000), //25MHz */
	/* FRATE(0, "pll0",   NULL, 0, 400000000), //400MHz */
	/* FRATE(0, "pll1",   NULL, 0, 400000000), //400MHz */
	FRATE(0, "p_cpu0_core",     NULL, 0, 400000000), //400MHz
	FRATE(0, "p_sys0_clk400",   NULL, 0, 400000000), //400MHz
};

/* fixed factor clocks */
static const struct nexell_fixed_factor_clock swallow_fixed_factor_clks[] __initconst = {
	FFACTOR(CLK_SYS_DIV_CPU0_CORE,   "div_cpu0_core",  "p_cpu0_core",   1,  2, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_AXI   , "div_sys0_axi",   "p_sys0_clk400", 1,  2, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_APB   , "div_sys0_apb",   "p_sys0_clk400", 1,  4, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_CLK400, "div_sys0_clk400","p_sys0_clk400", 1,  1, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_CLK133, "div_sys0_clk133","p_sys0_clk400", 1,  3, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_CLK50 , "div_sys0_clk50", "p_sys0_clk400", 1,  8, 0),
	FFACTOR(CLK_SYS_DIV_SYS0_CLK40 , "div_sys0_clk40", "p_sys0_clk400", 1, 10, 0),
};

/* gate clocks */
static const struct nexell_gate_clock swallow_gate_clks[] __initconst = {
	GATE(CLK_CPU_CORE, "cpu_core", "div_cpu0_core",  CPU0_CORE + 0x0C, 0, 0, 0),

	//CLK0
	GATE(CLK_0_SYS0_CLK400 , "sys0_clk400", "div_sys0_clk400",   SYS0_CLK400 + 0x0C, 0, 0, 0),
	GATE(CLK_0_DDRC0_DDRC  , "ddrc0_ddrc",  "div_sys0_clk400",   SYS0_CLK400 + 0x0C, 1, 0, 0),
	GATE(CLK_0_OSDMMC0_CORE, "osdmmc0_core","div_sys0_clk400",   SYS0_CLK400 + 0x0C, 2, 0, 0),
	GATE(CLK_0_OSDMMC1_CORE, "osdmmc1_core","div_sys0_clk400",   SYS0_CLK400 + 0x0C, 3, 0, 0),
	GATE(CLK_0_SYS0_AXI    , "sys0_axi",    "div_sys0_axi",   SYS0_CLK400 + 0x0C, 4, 0, 0),
	GATE(CLK_0_SYSBUS0_AXI , "sysbus0_axi", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 5, 0, 0),
	GATE(CLK_0_AXISRAM0_AXI, "axisram0_axi","div_sys0_axi",   SYS0_CLK400 + 0x0C, 6, 0, 0),
	GATE(CLK_0_VIP0_AXI    , "vip0_axi",    "div_sys0_axi",   SYS0_CLK400 + 0x0C, 7, 0, 0),
	GATE(CLK_0_SPI0_CORE   , "spi0_core",   "div_sys0_axi",   SYS0_CLK400 + 0x0C, 8, 0, 0),
	GATE(CLK_0_SCALER0_AXI , "scaler0_axi", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 9, 0, 0),
	GATE(CLK_0_SCALER0_APB , "scaler0_apb", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 10, 0, 0),
	GATE(CLK_0_SPI1_CORE   , "spi1_core",   "div_sys0_axi",   SYS0_CLK400 + 0x0C, 11, 0, 0),
	GATE(CLK_0_SPI2_CORE   , "spi2_core",   "div_sys0_axi",   SYS0_CLK400 + 0x0C, 12, 0, 0),
	GATE(CLK_0_QSPI0_CORE  , "qspi0_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 13, 0, 0),
	GATE(CLK_0_QSPI1_CORE  , "qspi1_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 14, 0, 0),
	GATE(CLK_0_DMA0_AXI    , "dma0_axi",    "div_sys0_axi",   SYS0_CLK400 + 0x0C, 15, 0, 0),
	GATE(CLK_0_QSPI2_CORE  , "qspi2_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 16, 0, 0),
	GATE(CLK_0_DMA1_AXI    , "dma1_axi",    "div_sys0_axi",   SYS0_CLK400 + 0x0C, 17, 0, 0),
	GATE(CLK_0_QSPI3_CORE  , "qspi3_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 18, 0, 0),
	GATE(CLK_0_HOVER0_AXI  , "hover0_axi",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 19, 0, 0),
	GATE(CLK_0_QSPI4_CORE  , "qspi4_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 20, 0, 0),
	GATE(CLK_0_UART0_CORE  , "uart0_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 21, 0, 0),
	GATE(CLK_0_UART1_CORE  , "uart1_core",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 22, 0, 0),
	GATE(CLK_0_USART0_CORE , "usart0_core", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 23, 0, 0),
	GATE(CLK_0_USART1_CORE , "usart1_core", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 24, 0, 0),
	GATE(CLK_0_USART2_CORE , "usart2_core", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 25, 0, 0),
	GATE(CLK_0_USART3_CORE , "usart3_core", "div_sys0_axi",   SYS0_CLK400 + 0x0C, 26, 0, 0),
	GATE(CLK_0_PWM0_TCLK0  , "pwm0_tclk0",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 27, 0, 0),
	GATE(CLK_0_PWM0_TCLK1  , "pwm0_tclk1",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 28, 0, 0),
	GATE(CLK_0_PWM0_TCLK2  , "pwm0_tclk2",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 29, 0, 0),
	GATE(CLK_0_PWM0_TCLK3  , "pwm0_tclk3",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 30, 0, 0),
	GATE(CLK_0_PWM1_TCLK0  , "pwm1_tclk0",  "div_sys0_axi",   SYS0_CLK400 + 0x0C, 31, 0, 0),

	//CLK1
	GATE(CLK_1_PWM1_TCLK1   , "pwm1_tclk1",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 0, 0, 0),
	GATE(CLK_1_PWM1_TCLK2   , "pwm1_tclk2",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 1, 0, 0),
	GATE(CLK_1_PWM1_TCLK3   , "pwm1_tclk3",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 2, 0, 0),
	GATE(CLK_1_PWM2_TCLK0   , "pwm2_tclk0",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 3, 0, 0),
	GATE(CLK_1_PWM2_TCLK1   , "pwm2_tclk1",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 4, 0, 0),
	GATE(CLK_1_PWM2_TCLK2   , "pwm2_tclk2",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 5, 0, 0),
	GATE(CLK_1_PWM2_TCLK3   , "pwm2_tclk3",  "div_sys0_axi",   SYS0_CLK400 + 0x10, 6, 0, 0),
	GATE(CLK_1_USB0_AHB     , "usb0_ahb",    "div_sys0_axi",   SYS0_CLK400 + 0x10, 7, 0, 0),
	GATE(CLK_1_TIMER0_TCLK0 , "timer0_tclk0","div_sys0_axi",   SYS0_CLK400 + 0x10, 8, 0, 0),
	GATE(CLK_1_TIMER0_TCLK1 , "timer0_tclk1","div_sys0_axi",   SYS0_CLK400 + 0x10, 9, 0, 0),
	GATE(CLK_1_TIMER0_TCLK2 , "timer0_tclk2","div_sys0_axi",   SYS0_CLK400 + 0x10, 10, 0, 0),
	GATE(CLK_1_TIMER0_TCLK3 , "timer0_tclk3","div_sys0_axi",   SYS0_CLK400 + 0x10, 11, 0, 0),
	GATE(CLK_1_TIMER1_TCLK0 , "timer1_tclk0","div_sys0_axi",   SYS0_CLK400 + 0x10, 12, 0, 0),
	GATE(CLK_1_TIMER1_TCLK1 , "timer1_tclk1","div_sys0_axi",   SYS0_CLK400 + 0x10, 13, 0, 0),
	GATE(CLK_1_TIMER1_TCLK2 , "timer1_tclk2","div_sys0_axi",   SYS0_CLK400 + 0x10, 14, 0, 0),
	GATE(CLK_1_TIMER1_TCLK3 , "timer1_tclk3","div_sys0_axi",   SYS0_CLK400 + 0x10, 15, 0, 0),
	GATE(CLK_1_OSDMMC0_AHB  , "osdmmc0_ahb", "div_sys0_axi",   SYS0_CLK400 + 0x10, 16, 0, 0),
	GATE(CLK_1_OSDMMC1_AHB  , "osdmmc1_ahb", "div_sys0_axi",   SYS0_CLK400 + 0x10, 17, 0, 0),
	GATE(CLK_1_SYS0_APB     , "sys0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x10, 18, 0, 0),
	GATE(CLK_1_SYSBUS0_APB  , "sysbus0_apb", "div_sys0_apb",   SYS0_CLK400 + 0x10, 19, 0, 0),
	GATE(CLK_1_VIP0_APB     , "vip0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x10, 20, 0, 0),
	GATE(CLK_1_VIP0_PADOUT0 , "vip0_padout0","div_sys0_apb",   SYS0_CLK400 + 0x10, 21, 0, 0),
	GATE(CLK_1_VIP0_PADOUT1 , "vip0_padout1","div_sys0_apb",   SYS0_CLK400 + 0x10, 22, 0, 0),
	GATE(CLK_1_SPI0_APB     , "spi0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x10, 23, 0, 0),
	GATE(CLK_1_C2DL0_AHB    , "c2dl0_ahb",   "div_sys0_apb",   SYS0_CLK400 + 0x10, 24, 0, 0),
	GATE(CLK_1_SPI1_APB     , "spi1_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x10, 25, 0, 0),
	GATE(CLK_1_CODA0_APB    , "coda0_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x10, 26, 0, 0),
	GATE(CLK_1_SPI2_APB     , "spi2_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x10, 27, 0, 0),
	GATE(CLK_1_QSPI0_APB    , "qspi0_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x10, 28, 0, 0),
	GATE(CLK_1_SYSREG_SYS0_APB, "sysreg_sys0_apb","div_sys0_apb",SYS0_CLK400 + 0x10, 29, 0, 0),
	GATE(CLK_1_CRYPTO0_APB  , "crypto0_apb", "div_sys0_apb",   SYS0_CLK400 + 0x10, 30, 0, 0),
	GATE(CLK_1_QSPI1_APB    , "qspi1_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x10, 31, 0, 0),

	//CLK2
	GATE(CLK_2_DMA0_APB    , "dma0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 0, 0, 0),
	GATE(CLK_2_QSPI2_APB   , "qspi2_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 1, 0, 0),
	GATE(CLK_2_DMA1_APB    , "dma1_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 2, 0, 0),
	GATE(CLK_2_QSPI3_APB   , "qspi3_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 3, 0, 0),
	GATE(CLK_2_HOVER0_HOVER_SYS, "hover0_hover_sys", "div_sys0_apb",   SYS0_CLK400 + 0x14, 4, 0, 0),
	GATE(CLK_2_QSPI4_APB   , "qspi4_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 5, 0, 0),
	GATE(CLK_2_UART0_APB   , "uart0_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 6, 0, 0),
	GATE(CLK_2_UART1_APB   , "uart1_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 7, 0, 0),
	GATE(CLK_2_USART0_APB  , "usart0_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 8, 0, 0),
	GATE(CLK_2_USART1_APB  , "usart1_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 9, 0, 0),
	GATE(CLK_2_USART2_APB  , "usart2_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 10, 0, 0),
	GATE(CLK_2_USART3_APB  , "usart3_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 11, 0, 0),
	GATE(CLK_2_PWM0_APB    , "pwm0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 12, 0, 0),
	GATE(CLK_2_PWM1_APB    , "pwm1_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 13, 0, 0),
	GATE(CLK_2_PWM2_APB    , "pwm2_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 14, 0, 0),
	GATE(CLK_2_TIMER0_APB  , "timer0_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 15, 0, 0),
	GATE(CLK_2_TIMER1_APB  , "timer1_apb",  "div_sys0_apb",   SYS0_CLK400 + 0x14, 16, 0, 0),
	GATE(CLK_2_I2C0_APB    , "i2c0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 17, 0, 0),
	GATE(CLK_2_TRIKBOX0_APB, "trikbox0_apb","div_sys0_apb",   SYS0_CLK400 + 0x14, 18, 0, 0),
	GATE(CLK_2_I2C1_APB    , "i2c1_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 19, 0, 0),
	GATE(CLK_2_I2C2_APB    , "i2c2_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 20, 0, 0),
	GATE(CLK_2_I2C3_APB    , "i2c3_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 21, 0, 0),
	GATE(CLK_2_I2C4_APB    , "i2c4_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 22, 0, 0),
	GATE(CLK_2_I2C5_APB    , "i2c5_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 23, 0, 0),
	GATE(CLK_2_I2C6_APB    , "i2c6_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 24, 0, 0),
	GATE(CLK_2_I2C7_APB    , "i2c7_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 25, 0, 0),
	GATE(CLK_2_I2C8_APB    , "i2c8_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 26, 0, 0),
	GATE(CLK_2_I2C9_APB    , "i2c9_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 27, 0, 0),
	GATE(CLK_2_I2C10_APB   , "i2c10_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 28, 0, 0),
	GATE(CLK_2_I2C11_APB   , "i2c11_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x14, 29, 0, 0),
	GATE(CLK_2_ADC0_APB    , "adc0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 30, 0, 0),
	GATE(CLK_2_WDT0_APB    , "wdt0_apb",    "div_sys0_apb",   SYS0_CLK400 + 0x14, 31, 0, 0),

	//CLK3
	GATE(CLK_3_WDT0_POR   , "wdt0_por",    "div_sys0_apb",   SYS0_CLK400 + 0x18, 0, 0, 0),
	GATE(CLK_3_GPIO0_APB  , "gpio0_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 1, 0, 0),
	GATE(CLK_3_GPIO1_APB  , "gpio1_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 2, 0, 0),
	GATE(CLK_3_GPIO2_APB  , "gpio2_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 3, 0, 0),
	GATE(CLK_3_GPIO3_APB  , "gpio3_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 4, 0, 0),
	GATE(CLK_3_GPIO4_APB  , "gpio4_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 5, 0, 0),
	GATE(CLK_3_GPIO5_APB  , "gpio5_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 6, 0, 0),
	GATE(CLK_3_GPIO6_APB  , "gpio6_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 7, 0, 0),
	GATE(CLK_3_GPIO7_APB  , "gpio7_apb",   "div_sys0_apb",   SYS0_CLK400 + 0x18, 8, 0, 0),
	GATE(CLK_3_SYS0_CLK133, "sys0_clk133", "div_sys0_clk133",   SYS0_CLK400 + 0x18, 9, 0, 0),
	GATE(CLK_3_CODA0_AXI  , "coda0_axi",   "div_sys0_clk133",   SYS0_CLK400 + 0x18, 10, 0, 0),
	GATE(CLK_3_CODA0_CORE , "coda0_core",  "div_sys0_clk133",   SYS0_CLK400 + 0x18, 11, 0, 0),
	GATE(CLK_3_SYS0_CLK50 , "sys0_clk50",  "div_sys0_clk50",   SYS0_CLK400 + 0x18, 12, 0, 0),
	GATE(CLK_3_RSP0_CORE  , "rsp0_core",   "div_sys0_clk50",   SYS0_CLK400 + 0x18, 13, 0, 0),
	GATE(CLK_3_RSP0_APB   , "rsp0_apb",    "div_sys0_clk50",   SYS0_CLK400 + 0x18, 14, 0, 0),
	GATE(CLK_3_HOVER0_APB , "hover0_apb",  "div_sys0_clk50",   SYS0_CLK400 + 0x18, 15, 0, 0),
	GATE(CLK_3_HOVER0_HOVER_CAM, "hover0_hover_cam", "div_sys0_clk50",   SYS0_CLK400 + 0x18, 16, 0, 0),
	GATE(CLK_3_SYS0_CLK40 , "sys0_clk40",  "div_sys0_clk40",   SYS0_CLK400 + 0x18, 17, 0, 0),
	GATE(CLK_3_C2DL0_CORE , "c2dl0_core",  "div_sys0_clk40",   SYS0_CLK400 + 0x18, 18, 0, 0),
};

static int swallow_clk_restart_notify(struct notifier_block *this,
		unsigned long code, void *unused)
{
	u32 val, status;

	status = readl_relaxed(reg_base + 0xbc);
	val = readl_relaxed(reg_base + 0xcc);
	val = (val & 0xffff0000) | (status & 0xffff);
	writel_relaxed(val, reg_base + 0xcc);

	return NOTIFY_DONE;
}

/*
 * Swallow Clock restart notifier, handles restart functionality
 */
static struct notifier_block swallow_clk_restart_handler = {
	.notifier_call = swallow_clk_restart_notify,
	.priority = 128,
};

/* register swallow clocks */
static void __init swallow_clk_init(struct device_node *np)
{
	struct nexell_clk_provider *ctx;

	reg_base = of_iomap(np, 0);
	if (!reg_base) {
		pr_err("%s: failed to map clock controller registers,"
			" aborting clock initialization\n", __func__);
		return;
	}

	ctx = nexell_clk_init(np, reg_base, CLK_NR_CLKS);

	nexell_clk_register_fixed_rate(ctx, swallow_fixed_rate_clks,
			ARRAY_SIZE(swallow_fixed_rate_clks));
	nexell_clk_register_fixed_factor(ctx, swallow_fixed_factor_clks,
			ARRAY_SIZE(swallow_fixed_factor_clks));
	nexell_clk_register_gate(ctx, swallow_gate_clks,
			ARRAY_SIZE(swallow_gate_clks));

	nexell_clk_of_add_provider(np, ctx);

	if (register_restart_handler(&swallow_clk_restart_handler))
		pr_warn("swallow clock can't register restart handler\n");

	pr_info("swallow clock initialization complete\n");
}
CLK_OF_DECLARE_DRIVER(swallow_cmu, "nexell,swallow-cmu", swallow_clk_init);
