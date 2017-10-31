#
# Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#


LOAD_IMAGE_V2	:=	1

COLD_BOOT_SINGLE_CPU		:=	1
PROGRAMMABLE_RESET_ADDRESS	:=	1


DESIGN_COMPANY  := designreview
DESIGN_PLAT     := h960
DESIGN_ROOT		:= plat/$(DESIGN_COMPANY)/$(DESIGN_PLAT)
PLAT_INCLUDES	:=	-Iinclude/common/tbbr			\
				-I$(DESIGN_ROOT)/          \
				-I$(DESIGN_ROOT)/include

ENABLE_PLAT_COMPAT	:=	0

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/pl011_console.S	\
				drivers/delay_timer/delay_timer.c	\
				drivers/delay_timer/generic_delay_timer.c \
				lib/aarch64/xlat_tables.c		\

H960_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v2/gicv2_main.c		\
				drivers/arm/gic/v2/gicv2_helpers.c	\
				plat/common/plat_gicv2.c

BL1_SOURCES		+=	bl1/tbbr/tbbr_img_desc.c		\
				drivers/io/io_fip.c			\
				drivers/io/io_storage.c			\
				drivers/synopsys/ufs/dw_ufs.c		\
				drivers/io/io_memmap.c \
				lib/cpus/aarch64/cortex_a53.S		\
				$(DESIGN_ROOT)/h960_bl1_setup.c     \
				$(DESIGN_ROOT)/h960_io_storage.c    \
				$(DESIGN_ROOT)/h960_common.c    \
				$(DESIGN_ROOT)/aarch64/h960_helper.S

BL2_SOURCES		+=	\
				drivers/io/io_fip.c					\
				drivers/io/io_storage.c				\
				common/desc_image_load.c        	\
				drivers/io/io_memmap.c \
				$(DESIGN_ROOT)/h960_bl2_setup.c 	\
				$(DESIGN_ROOT)/h960_common.c    \
				$(DESIGN_ROOT)/h960_io_storage.c    \
				$(DESIGN_ROOT)/h960_bl2_mem_params_desc.c    \
				$(DESIGN_ROOT)/h960_mcu_load.c    \


BL31_SOURCES		+=	drivers/arm/cci/cci.c			\
				lib/cpus/aarch64/cortex_a53.S           \
				lib/cpus/aarch64/cortex_a72.S		\
				lib/cpus/aarch64/cortex_a73.S		\
				plat/common/aarch64/plat_psci_common.c  \
				$(DESIGN_ROOT)/h960_common.c    \
				$(DESIGN_ROOT)/aarch64/h960_helper.S    \
				$(DESIGN_ROOT)/h960_bl31_setup.c \
				$(DESIGN_ROOT)/h960_topology.c \
				$(DESIGN_ROOT)/h960_pm.c \
				${H960_GIC_SOURCES}

		#		plat/hisilicon/hikey960/hikey960_pm.c	\
				plat/hisilicon/hikey960/hikey960_topology.c \
				plat/hisilicon/hikey960/drivers/pwrc/hisi_pwrc.c \
				plat/hisilicon/hikey960/drivers/ipc/hisi_ipc.c \
				${H960_GIC_SOURCES}

# Enable workarounds for selected Cortex-A53 errata.
ERRATA_A53_836870		:=	1
ERRATA_A53_843419		:=	1
ERRATA_A53_855873		:=	1
