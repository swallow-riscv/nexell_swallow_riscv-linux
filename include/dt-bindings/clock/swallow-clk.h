/* SPDX-License-Identifier: (GPL-2.0+ or MIT) */
/*
 * Nexell SWALLOW SoC clock tree IDs
 * Copyright (c) 2018 Choonghyun Jeon <suker@nexell.co.kr>
 */

#ifndef _DT_BINDINGS_CLOCK_NEXELL_SWALLOW_CLOCK_H
#define _DT_BINDINGS_CLOCK_NEXELL_SWALLOW_CLOCK_H

/*
 * CMU SRC
 */
#define CLK_PLL0_DIV		1
#define CLK_PLL1_DIV		2
#define CLK_EXT_SRC		3

/*
 * CMU SYS
 */

/* Dividers */
#define CLK_SYS_DIV_CPU0_CORE		4
#define CLK_SYS_DIV_SYS0_AXI            5
#define CLK_SYS_DIV_SYS0_APB            6
#define CLK_SYS_DIV_SYS0_CLK400         7
#define CLK_SYS_DIV_SYS0_CLK133         8
#define CLK_SYS_DIV_SYS0_CLK50          9
#define CLK_SYS_DIV_SYS0_CLK40         10

/* Gates */
#define CLK_CPU_CORE                   11

//CLK0
#define CLK_0_SYS0_CLK400              12
#define CLK_0_DDRC0_DDRC               13
#define CLK_0_OSDMMC0_CORE             14
#define CLK_0_OSDMMC1_CORE             15
#define CLK_0_SYS0_AXI                 16
#define CLK_0_SYSBUS0_AXI              17
#define CLK_0_AXISRAM0_AXI             18
#define CLK_0_VIP0_AXI                 19
#define CLK_0_SPI0_CORE                20
#define CLK_0_SCALER0_AXI              21
#define CLK_0_SCALER0_APB              22
#define CLK_0_SPI1_CORE                23
#define CLK_0_SPI2_CORE                24
#define CLK_0_QSPI0_CORE               25
#define CLK_0_QSPI1_CORE               26
#define CLK_0_DMA0_AXI                 27
#define CLK_0_QSPI2_CORE               28
#define CLK_0_DMA1_AXI                 29
#define CLK_0_QSPI3_CORE               30
#define CLK_0_HOVER0_AXI               31
#define CLK_0_QSPI4_CORE               32
#define CLK_0_UART0_CORE               33
#define CLK_0_UART1_CORE               34
#define CLK_0_USART0_CORE              35
#define CLK_0_USART1_CORE              36
#define CLK_0_USART2_CORE              37
#define CLK_0_USART3_CORE              38
#define CLK_0_PWM0_TCLK0               39
#define CLK_0_PWM0_TCLK1               40
#define CLK_0_PWM0_TCLK2               41
#define CLK_0_PWM0_TCLK3               42
#define CLK_0_PWM1_TCLK0               43

//CLK1
#define CLK_1_PWM1_TCLK1               44
#define CLK_1_PWM1_TCLK2               45
#define CLK_1_PWM1_TCLK3               46
#define CLK_1_PWM2_TCLK0               47
#define CLK_1_PWM2_TCLK1               48
#define CLK_1_PWM2_TCLK2               49
#define CLK_1_PWM2_TCLK3               50
#define CLK_1_USB0_AHB                 51
#define CLK_1_TIMER0_TCLK0             52
#define CLK_1_TIMER0_TCLK1             53
#define CLK_1_TIMER0_TCLK2             54
#define CLK_1_TIMER0_TCLK3             55
#define CLK_1_TIMER1_TCLK0             56
#define CLK_1_TIMER1_TCLK1             57
#define CLK_1_TIMER1_TCLK2             58
#define CLK_1_TIMER1_TCLK3             59
#define CLK_1_OSDMMC0_AHB              60
#define CLK_1_OSDMMC1_AHB              61
#define CLK_1_SYS0_APB                 62
#define CLK_1_SYSBUS0_APB              63
#define CLK_1_VIP0_APB                 64
#define CLK_1_VIP0_PADOUT0             65
#define CLK_1_VIP0_PADOUT1             66
#define CLK_1_SPI0_APB                 67
#define CLK_1_C2DL0_AHB                68
#define CLK_1_SPI1_APB                 69
#define CLK_1_CODA0_APB                70
#define CLK_1_SPI2_APB                 71
#define CLK_1_QSPI0_APB                72
#define CLK_1_SYSREG_SYS0_APB          73
#define CLK_1_CRYPTO0_APB              74
#define CLK_1_QSPI1_APB                75

//CLK2
#define CLK_2_DMA0_APB                 76
#define CLK_2_QSPI2_APB                77
#define CLK_2_DMA1_APB                 78
#define CLK_2_QSPI3_APB                79
#define CLK_2_HOVER0_HOVER_SYS         80
#define CLK_2_QSPI4_APB                81
#define CLK_2_UART0_APB                82
#define CLK_2_UART1_APB                83
#define CLK_2_USART0_APB               84
#define CLK_2_USART1_APB               85
#define CLK_2_USART2_APB               86
#define CLK_2_USART3_APB               87
#define CLK_2_PWM0_APB                 88
#define CLK_2_PWM1_APB                 89
#define CLK_2_PWM2_APB                 90
#define CLK_2_TIMER0_APB               91
#define CLK_2_TIMER1_APB               92
#define CLK_2_I2C0_APB                 93
#define CLK_2_TRIKBOX0_APB             94
#define CLK_2_I2C1_APB                 95
#define CLK_2_I2C2_APB                 96
#define CLK_2_I2C3_APB                 97
#define CLK_2_I2C4_APB                 98
#define CLK_2_I2C5_APB                 99
#define CLK_2_I2C6_APB                100
#define CLK_2_I2C7_APB                101
#define CLK_2_I2C8_APB                102
#define CLK_2_I2C9_APB                103
#define CLK_2_I2C10_APB               104
#define CLK_2_I2C11_APB               105
#define CLK_2_ADC0_APB                106
#define CLK_2_WDT0_APB                107

//CLK3
#define CLK_3_WDT0_POR                108
#define CLK_3_GPIO0_APB               109
#define CLK_3_GPIO1_APB               110
#define CLK_3_GPIO2_APB               111
#define CLK_3_GPIO3_APB               112
#define CLK_3_GPIO4_APB               113
#define CLK_3_GPIO5_APB               114
#define CLK_3_GPIO6_APB               115
#define CLK_3_GPIO7_APB               116
#define CLK_3_SYS0_CLK133             117
#define CLK_3_CODA0_AXI               118
#define CLK_3_CODA0_CORE              119
#define CLK_3_SYS0_CLK50              120
#define CLK_3_RSP0_CORE               121
#define CLK_3_RSP0_APB                122
#define CLK_3_HOVER0_APB              123
#define CLK_3_HOVER0_HOVER_CAM        124
#define CLK_3_SYS0_CLK40              125
#define CLK_3_C2DL0_CORE              126

#define CLK_NR_CLKS                   127

#endif
