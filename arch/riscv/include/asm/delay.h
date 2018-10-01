/*
 * Copyright (C) 2009 Chen Liqin <liqin.chen@sunplusct.com>
 * Copyright (C) 2016 Regents of the University of California
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#ifndef _ASM_RISCV_DELAY_H
#define _ASM_RISCV_DELAY_H

extern unsigned long riscv_timebase;

#define udelay udelay
extern void udelay(unsigned long usecs);

#define ndelay ndelay
extern void ndelay(unsigned long nsecs);

extern void __delay(unsigned long cycles);

#ifndef CONFIG_RISCV_TIMER
struct delay_timer {
        unsigned long (*read_current_timer)(void);
        unsigned long freq;
};

extern void register_current_timer_delay(const struct delay_timer *timer);
#endif

#endif /* _ASM_RISCV_DELAY_H */
