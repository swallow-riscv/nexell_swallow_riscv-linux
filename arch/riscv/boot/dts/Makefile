# SPDX-License-Identifier: GPL-2.0
ifeq ($(CONFIG_OF),y)

dtb-$(CONFIG_RISCV) += \
	swallow.dtb
endif

dtstree		:= $(srctree)/$(src)
always		:= $(dtb-y)
clean-files	:= *.dtb
