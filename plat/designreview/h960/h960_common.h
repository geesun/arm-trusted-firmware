
#ifndef __H960_COMMON_H__
#define __H960_COMMON_H__

#include <bl_common.h>

/*
 * Function and variable prototypes
 */
void h960_init_mmu_el1(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit,
			unsigned long coh_start,
			unsigned long coh_limit);
void h960_init_mmu_el3(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit,
			unsigned long coh_start,
			unsigned long coh_limit);
#endif
