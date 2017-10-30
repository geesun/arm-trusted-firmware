
/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__
#include <arch.h>
#include <common_def.h>
#include <tbbr_img_def.h>

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE		0x800

/* 
 * Core relate information 
 */
#define PLATFORM_CLUSTER_COUNT				2
#define PLATFORM_CORE_COUNT_PER_CLUSTER		4
#define PLATFORM_CORE_COUNT					(PLATFORM_CLUSTER_COUNT * \
					 						  PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLAT_MAX_PWR_LVL 				    MPIDR_AFFLVL2
#define PLAT_NUM_PWR_DOMAINS                (PLATFORM_CORE_COUNT + \
					 						  PLATFORM_CLUSTER_COUNT + 1)

/* 
 * IO relate define 
 */
#define MAX_IO_DEVICES						3
#define MAX_IO_HANDLES						4
/* UFS RPMB and UFS User Data */
#define MAX_IO_BLOCK_DEVICES				2

/*
 * Platform memory map related constants
 */

/*
 * BL1 specific defines.
 */
#define BL1_RO_BASE			(0x1AC00000)
#define BL1_RO_LIMIT			(BL1_RO_BASE + 0x10000)
#define BL1_RW_BASE			(BL1_RO_LIMIT)		/* 1AC1_0000 */
#define BL1_RW_SIZE			(0x00188000)
#define BL1_RW_LIMIT			(0x1B000000)

/*
 * BL2 specific defines.
 */
#define BL2_BASE			(BL1_RW_BASE + 0x8000)	/* 1AC1_8000 */
#define BL2_LIMIT			(BL2_BASE + 0x40000)	/* 1AC5_8000 */

/*
 * BL31 specific defines.
 */
#define BL31_BASE			(BL2_LIMIT)		/* 1AC5_8000 */
#define BL31_LIMIT			(BL31_BASE + 0x40000)	/* 1AC9_8000 */



/*
 * Platform specific page table and MMU setup constants
 */
#define ADDR_SPACE_SIZE			(1ull << 32)

#if IMAGE_BL1 || IMAGE_BL31 || IMAGE_BL32
#define MAX_XLAT_TABLES			3
#endif

#if IMAGE_BL2
#if LOAD_IMAGE_V2
#ifdef SPD_opteed
#define MAX_XLAT_TABLES			4
#else
#define MAX_XLAT_TABLES			3
#endif
#else
#define MAX_XLAT_TABLES			3
#endif
#endif

#define MAX_MMAP_REGIONS		16



/*
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 */
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)
#endif
