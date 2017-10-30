#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <xlat_tables.h>

#define MAP_DDR		MAP_REGION_FLAT(DDR_BASE,			\
					DDR_SIZE,			\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_DEVICE	MAP_REGION_FLAT(DEVICE_BASE,			\
					DEVICE_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_BL1_RW	MAP_REGION_FLAT(BL1_RW_BASE,			\
					BL1_RW_LIMIT - BL1_RW_BASE,	\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_UFS_DATA	MAP_REGION_FLAT(H960_UFS_DATA_BASE,		\
					H960_UFS_DATA_SIZE,		\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_UFS_DESC	MAP_REGION_FLAT(H960_UFS_DESC_BASE,		\
					H960_UFS_DESC_SIZE,		\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_TSP_MEM	MAP_REGION_FLAT(TSP_SEC_MEM_BASE,		\
					TSP_SEC_MEM_SIZE,		\
					MT_MEMORY | MT_RW | MT_SECURE)

#if LOAD_IMAGE_V2
#ifdef SPD_opteed
#define MAP_OPTEE_PAGEABLE	MAP_REGION_FLAT(		\
					H960_OPTEE_PAGEABLE_LOAD_BASE,	\
					H960_OPTEE_PAGEABLE_LOAD_SIZE,	\
					MT_MEMORY | MT_RW | MT_SECURE)
#endif
#endif

/*
 * Table of regions for different BL stages to map using the MMU.
 * This doesn't include Trusted RAM as the 'mem_layout' argument passed to
 * h960_init_mmu_elx() will give the available subset of that,
 */
#if IMAGE_BL1
static const mmap_region_t h960_mmap[] = {
	MAP_UFS_DATA,
	MAP_BL1_RW,
	MAP_UFS_DESC,
	MAP_DEVICE,
	{0}
};
#endif

#if IMAGE_BL2
static const mmap_region_t h960_mmap[] = {
	MAP_DDR,
	MAP_DEVICE,
	MAP_TSP_MEM,
#if LOAD_IMAGE_V2
#ifdef SPD_opteed
	MAP_OPTEE_PAGEABLE,
#endif
#endif
	{0}
};
#endif

#if IMAGE_BL31
static const mmap_region_t h960_mmap[] = {
	MAP_DEVICE,
	MAP_TSP_MEM,
	{0}
};
#endif

#if IMAGE_BL32
static const mmap_region_t h960_mmap[] = {
	MAP_DEVICE,
	MAP_DDR,
	{0}
};
#endif

/*
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 */
#define H960_CONFIGURE_MMU_EL(_el)					\
	void h960_init_mmu_el##_el(unsigned long total_base,	\
				unsigned long total_size,		\
				unsigned long ro_start,			\
				unsigned long ro_limit,			\
				unsigned long coh_start,		\
				unsigned long coh_limit)		\
	{								\
	       mmap_add_region(total_base, total_base,			\
			       total_size,				\
			       MT_MEMORY | MT_RW | MT_SECURE);		\
	       mmap_add_region(ro_start, ro_start,			\
			       ro_limit - ro_start,			\
			       MT_MEMORY | MT_RO | MT_SECURE);		\
	       mmap_add_region(coh_start, coh_start,			\
			       coh_limit - coh_start,			\
			       MT_DEVICE | MT_RW | MT_SECURE);		\
	       mmap_add(h960_mmap);					\
	       init_xlat_tables();					\
									\
	       enable_mmu_el##_el(0);					\
	}

/* Define EL1 and EL3 variants of the function initialising the MMU */
H960_CONFIGURE_MMU_EL(1)
H960_CONFIGURE_MMU_EL(3)


unsigned int plat_get_syscnt_freq2(void)
{
	return 1920000;
}
