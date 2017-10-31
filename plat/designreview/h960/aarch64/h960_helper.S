#include <arch.h>
#include <asm_macros.S>
#include <cortex_a53.h>
#include <cortex_a73.h>

	.globl	platform_mem_init
	.globl	plat_my_core_pos

func platform_mem_init
	ret
endfunc platform_mem_init

func plat_my_core_pos
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_my_core_pos

