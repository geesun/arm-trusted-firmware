#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <dw_ufs.h>
#include <errno.h>
#include <generic_delay_timer.h>
#include <gicv2.h>
#include <mmio.h>
#include <hi3660.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <tbbr/tbbr_img_desc.h>
#include <ufs.h>

#include <platform_def.h>
#include <h960_common.h>
/*
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted RAM
 */
extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL1_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL1_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)

/* Data structure which holds the extents of the trusted RAM for BL1 */
static meminfo_t bl1_tzram_layout;

void bl1_init_bl2_mem_layout(const meminfo_t *bl1_mem_layout,
			     meminfo_t *bl2_mem_layout)
{

	assert(bl1_mem_layout != NULL);
	assert(bl2_mem_layout != NULL);

	/*
	 * Cannot remove BL1 RW data from the scope of memory visible to BL2
	 * like arm platforms because they overlap in hikey960
	 */
	bl2_mem_layout->total_base = BL2_BASE;
	bl2_mem_layout->total_size = BL31_LIMIT - BL2_BASE;

	flush_dcache_range((unsigned long)bl2_mem_layout, sizeof(meminfo_t));
}

void bl1_early_platform_setup(void)
{
	unsigned int uart_base;

	generic_delay_timer_init();
	uart_base = PL011_UART6_BASE;
	/* Initialize the console to provide early debug support */
	console_init(uart_base, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE);

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = BL1_RW_BASE;
	bl1_tzram_layout.total_size = BL1_RW_SIZE;
}

static void hikey960_peri_init(void)
{
	/* unreset */
	mmio_setbits_32(CRG_PERRSTDIS4_REG, 1);
}

static void hikey960_ufs_reset(void)
{
	unsigned int data, mask;

	mmio_write_32(CRG_PERDIS7_REG, 1 << 14);
	mmio_clrbits_32(UFS_SYS_PHY_CLK_CTRL_REG, BIT_SYSCTRL_REF_CLOCK_EN);
	do {
		data = mmio_read_32(UFS_SYS_PHY_CLK_CTRL_REG);
	} while (data & BIT_SYSCTRL_REF_CLOCK_EN);
	/* use abb clk */
	mmio_clrbits_32(UFS_SYS_UFS_SYSCTRL_REG, BIT_UFS_REFCLK_SRC_SE1);
	mmio_clrbits_32(UFS_SYS_PHY_ISO_EN_REG, BIT_UFS_REFCLK_ISO_EN);
	mmio_write_32(PCTRL_PERI_CTRL3_REG, (1 << 0) | (1 << 16));
	mdelay(1);
	mmio_write_32(CRG_PEREN7_REG, 1 << 14);
	mmio_setbits_32(UFS_SYS_PHY_CLK_CTRL_REG, BIT_SYSCTRL_REF_CLOCK_EN);

	mmio_write_32(CRG_PERRSTEN3_REG, PERI_UFS_BIT);
	do {
		data = mmio_read_32(CRG_PERRSTSTAT3_REG);
	} while ((data & PERI_UFS_BIT) == 0);
	mmio_setbits_32(UFS_SYS_PSW_POWER_CTRL_REG, BIT_UFS_PSW_MTCMOS_EN);
	mdelay(1);
	mmio_setbits_32(UFS_SYS_HC_LP_CTRL_REG, BIT_SYSCTRL_PWR_READY);
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG,
		      MASK_UFS_DEVICE_RESET);
	/* clear SC_DIV_UFS_PERIBUS */
	mask = SC_DIV_UFS_PERIBUS << 16;
	mmio_write_32(CRG_CLKDIV17_REG, mask);
	/* set SC_DIV_UFSPHY_CFG(3) */
	mask = SC_DIV_UFSPHY_CFG_MASK << 16;
	data = SC_DIV_UFSPHY_CFG(3);
	mmio_write_32(CRG_CLKDIV16_REG, mask | data);
	data = mmio_read_32(UFS_SYS_PHY_CLK_CTRL_REG);
	data &= ~MASK_SYSCTRL_CFG_CLOCK_FREQ;
	data |= 0x39;
	mmio_write_32(UFS_SYS_PHY_CLK_CTRL_REG, data);
	mmio_clrbits_32(UFS_SYS_PHY_CLK_CTRL_REG, MASK_SYSCTRL_REF_CLOCK_SEL);
	mmio_setbits_32(UFS_SYS_CLOCK_GATE_BYPASS_REG,
			MASK_UFS_CLK_GATE_BYPASS);
	mmio_setbits_32(UFS_SYS_UFS_SYSCTRL_REG, MASK_UFS_SYSCTRL_BYPASS);

	mmio_setbits_32(UFS_SYS_PSW_CLK_CTRL_REG, BIT_SYSCTRL_PSW_CLK_EN);
	mmio_clrbits_32(UFS_SYS_PSW_POWER_CTRL_REG, BIT_UFS_PSW_ISO_CTRL);
	mmio_clrbits_32(UFS_SYS_PHY_ISO_EN_REG, BIT_UFS_PHY_ISO_CTRL);
	mmio_clrbits_32(UFS_SYS_HC_LP_CTRL_REG, BIT_SYSCTRL_LP_ISOL_EN);
	mmio_write_32(CRG_PERRSTDIS3_REG, PERI_ARST_UFS_BIT);
	mmio_setbits_32(UFS_SYS_RESET_CTRL_EN_REG, BIT_SYSCTRL_LP_RESET_N);
	mdelay(1);
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG,
		      MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET);
	mdelay(20);
	mmio_write_32(UFS_SYS_UFS_DEVICE_RESET_CTRL_REG,
		      0x03300330);

	mmio_write_32(CRG_PERRSTDIS3_REG, PERI_UFS_BIT);
	do {
		data = mmio_read_32(CRG_PERRSTSTAT3_REG);
	} while (data & PERI_UFS_BIT);
}

static void hikey960_ufs_init(void)
{
	dw_ufs_params_t ufs_params;

	memset(&ufs_params, 0, sizeof(ufs_params));
	ufs_params.reg_base = UFS_REG_BASE;
	ufs_params.desc_base = H960_UFS_DESC_BASE;
	ufs_params.desc_size = H960_UFS_DESC_SIZE;

	if ((ufs_params.flags & UFS_FLAGS_SKIPINIT) == 0)
		hikey960_ufs_reset();
	dw_ufs_init(&ufs_params);
}

void bl1_plat_arch_setup(void)
{
	h960_init_mmu_el3(bl1_tzram_layout.total_base,
			      bl1_tzram_layout.total_size,
			      BL1_RO_BASE,
			      BL1_RO_LIMIT,
			      BL1_COHERENT_RAM_BASE,
			      BL1_COHERENT_RAM_LIMIT);
}
void bl1_platform_setup(void)
{
	hikey960_peri_init();
	hikey960_ufs_init();
	hi960_io_setup();
}
meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}
