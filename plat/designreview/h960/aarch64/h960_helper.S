#include <arch.h>
#include <asm_macros.S>
#include <cortex_a53.h>
#include <cortex_a73.h>

	.globl	platform_mem_init
	.globl	plat_my_core_pos
	.globl	set_retention_ticks
	.globl	clr_retention_ticks
	.globl	clr_ex
	.globl	nop

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


	/* -----------------------------------------------------
	 * void set_retention_ticks(unsigned int val);
	 * Clobber list : x0
	 * -----------------------------------------------------
	 */
func set_retention_ticks
	mrs	x0, CORTEX_A53_ECTLR_EL1
	bic	x0, x0, #CORTEX_A53_ECTLR_CPU_RET_CTRL_MASK
	orr	x0, x0, #RETENTION_ENTRY_TICKS_8
	msr	CORTEX_A53_ECTLR_EL1, x0
	isb
	dsb	sy
	ret
endfunc set_retention_ticks

	/* -----------------------------------------------------
	 * void clr_retention_ticks(unsigned int val);
	 * Clobber list : x0
	 * -----------------------------------------------------
	 */
func clr_retention_ticks
	mrs	x0, CORTEX_A53_ECTLR_EL1
	bic	x0, x0, #CORTEX_A53_ECTLR_CPU_RET_CTRL_MASK
	msr	CORTEX_A53_ECTLR_EL1, x0
	isb
	dsb	sy
	ret
endfunc clr_retention_ticks

	/* -----------------------------------------------------
	 * void clrex(void);
	 * -----------------------------------------------------
	 */
func clr_ex
	clrex
	ret
endfunc clr_ex


	/* -----------------------------------------------------
	 * void nop(void);
	 * -----------------------------------------------------
	 */
func nop
	nop
	ret
endfunc nop



