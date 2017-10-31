#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <desc_image_load.h>
#include <errno.h>
#include <generic_delay_timer.h>
#include <mmio.h>
#include <platform_def.h>
#include <string.h>
#include <h960_common.h>

extern  int load_lpm3(void);
/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define BL2_RO_BASE (unsigned long)(&__RO_START__)
#define BL2_RO_LIMIT (unsigned long)(&__RO_END__)

/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols refer to
 * page-aligned addresses.
 */
#define BL2_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL2_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)

static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

void bl2_early_platform_setup(meminfo_t *mem_layout)
{
	unsigned int uart_base;

	generic_delay_timer_init();
	uart_base = PL011_UART6_BASE;

	/* Initialize the console to provide early debug support */
	console_init(uart_base, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE);

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;
}

void bl2_plat_arch_setup(void)
{
	h960_init_mmu_el1(bl2_tzram_layout.total_base,
			      bl2_tzram_layout.total_size,
			      BL2_RO_BASE,
			      BL2_RO_LIMIT,
			      BL2_COHERENT_RAM_BASE,
			      BL2_COHERENT_RAM_LIMIT);
}

void bl2_platform_setup(void)
{

}

uint32_t hikey960_get_spsr_for_bl33_entry(void)
{
	unsigned int mode;
	uint32_t spsr;

	/* Figure out what mode we enter the non-secure world in */
	mode = EL_IMPLEMENTED(2) ? MODE_EL2 : MODE_EL1;

	/*
	 * TODO: Consider the possibility of specifying the SPSR in
	 * the FIP ToC and allowing the platform to have a say as
	 * well.
	 */
	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	return spsr;
}

int plat_h960_bl2_handle_scp_bl2(image_info_t *scp_bl2_image_info)
{
	int i;
	int *buf;

	assert(scp_bl2_image_info->image_size < SCP_BL2_SIZE);

	INFO("BL2: Initiating SCP_BL2 transfer to SCP\n");

	INFO("BL2: SCP_BL2: 0x%lx@0x%x\n",
	     scp_bl2_image_info->image_base,
	     scp_bl2_image_info->image_size);

	buf = (int *)scp_bl2_image_info->image_base;

	INFO("BL2: SCP_BL2 HEAD:\n");
	for (i = 0; i < 64; i += 4)
		INFO("BL2: SCP_BL2 0x%x 0x%x 0x%x 0x%x\n",
			buf[i], buf[i+1], buf[i+2], buf[i+3]);

	buf = (int *)(scp_bl2_image_info->image_base +
		      scp_bl2_image_info->image_size - 256);

	INFO("BL2: SCP_BL2 TAIL:\n");
	for (i = 0; i < 64; i += 4)
		INFO("BL2: SCP_BL2 0x%x 0x%x 0x%x 0x%x\n",
			buf[i], buf[i+1], buf[i+2], buf[i+3]);

	INFO("BL2: SCP_BL2 transferred to SCP\n");

	load_lpm3();
	(void)buf;

	return 0;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	int err = 0;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);
	assert(bl_mem_params);

	switch (image_id) {
	case BL33_IMAGE_ID:
		/* BL33 expects to receive the primary CPU MPID (through r0) */
		bl_mem_params->ep_info.args.arg0 = 0xffff & read_mpidr();
		bl_mem_params->ep_info.spsr = hikey960_get_spsr_for_bl33_entry();
		break;
#ifdef SCP_BL2_BASE
	case SCP_BL2_IMAGE_ID:
		/* The subsequent handling of SCP_BL2 is platform specific */
		err = plat_h960_bl2_handle_scp_bl2(&bl_mem_params->image_info);
		if (err) {
			WARN("Failure in platform-specific handling of SCP_BL2 image.\n");
		}
		break;
#endif
	}

	return err;
}

/*******************************************************************************
 * This function flushes the data structures so that they are visible
 * in memory for the next BL image.
 ******************************************************************************/
void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

/*******************************************************************************
 * This function returns the list of loadable images.
 ******************************************************************************/
bl_load_info_t *plat_get_bl_image_load_info(void)
{
	/* Required before loading scp_bl2 */
	hi960_io_setup();

	return get_bl_load_info_from_mem_params_desc();
}

/*******************************************************************************
 * This function returns the list of executable images.
 ******************************************************************************/
bl_params_t *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}
