
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

#if defined(IMAGE_BL1)
/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE		0x800
#elif defined(IMAGE_BL2)
#  define PLATFORM_STACK_SIZE 0x1000
#else 
#  define PLATFORM_STACK_SIZE 0x1000
#endif 
/*
 * PL011 related constants
 */
#define PL011_UART5_BASE		0xFDF05000
#define PL011_UART6_BASE		0xFFF32000
#define PL011_BAUDRATE			115200
#define PL011_UART_CLK_IN_HZ		19200000



#define DDR_BASE			0x0
#define DDR_SIZE			0xC0000000

#define DEVICE_BASE			0xE0000000
#define DEVICE_SIZE			0x20000000

/*
 * DDR for OP-TEE (32MB from 0x3E00000-0x3FFFFFFF) is divided in several
 * regions:
 *   - Secure DDR (default is the top 16MB) used by OP-TEE
 *   - Non-secure DDR used by OP-TEE (shared memory and padding) (4MB)
 *   - Secure DDR (4MB aligned on 4MB) for OP-TEE's "Secure Data Path" feature
 *   - Non-secure DDR (8MB) reserved for OP-TEE's future use
 */
#define DDR_SEC_SIZE			0x01000000
#define DDR_SEC_BASE			0x3F000000

#define DDR_SDP_SIZE			0x00400000
#define DDR_SDP_BASE			(DDR_SEC_BASE - 0x400000 /* align */ - \
								DDR_SDP_SIZE)


#define UFS_BASE			0
/* FIP partition */
#define H960_FIP_BASE		 	0x1ACA8000
#define H960_FIP_MAX_SIZE		(12 << 20)

#define H960_UFS_FIP_BASE		 	(UFS_BASE + 0x1400000)
#define H960_UFS_FIP_MAX_SIZE		(12 << 20)

#define H960_UFS_DESC_BASE		0x20000000
#define H960_UFS_DESC_SIZE		0x00200000	/* 2MB */
#define H960_UFS_DATA_BASE		0x10000000
#define H960_UFS_DATA_SIZE		0x0A000000	/* 160MB */

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
#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_OFF_STATE		2

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
#define BL1_RO_LIMIT			(BL1_RO_BASE + 0x20000)
#define BL1_RW_BASE			(BL1_RO_LIMIT)		/* 1AC2_0000 */
#define BL1_RW_SIZE			(0x00188000)
#define BL1_RW_LIMIT			(0x1B000000)

/*
 * BL2 specific defines.
 */
#define BL2_BASE			(BL1_RW_BASE + 0x8000)	/* 1AC2_8000 */
#define BL2_LIMIT			(BL2_BASE + 0x40000)	/* 1AC6_8000 */

/*
 * BL31 specific defines.
 */
#define BL31_BASE			(BL2_LIMIT)		/* 1AC6_8000 */
#define BL31_LIMIT			(BL31_BASE + 0x40000)	/* 1ACA_8000 */


/*
 * The TSP currently executes from TZC secured area of DRAM.
 */
#define BL32_DRAM_BASE                  DDR_SEC_BASE
#define BL32_DRAM_LIMIT                 (DDR_SEC_BASE+DDR_SEC_SIZE)

#define TSP_SEC_MEM_BASE		BL32_DRAM_BASE
#define TSP_SEC_MEM_SIZE		(BL32_DRAM_LIMIT - BL32_DRAM_BASE)
//#define BL32_BASE			BL32_DRAM_BASE
//#define BL32_LIMIT			BL32_DRAM_LIMIT

#define NS_BL1U_BASE			(BL31_LIMIT + 0x10000)		/* 1AC9_8000 */
#define NS_BL1U_SIZE			(0x00100000)
#define NS_BL1U_LIMIT			(NS_BL1U_BASE + NS_BL1U_SIZE)

#define SCP_BL2_BASE			(0x89C80000)
#define SCP_BL2_SIZE			(0x00040000)

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
