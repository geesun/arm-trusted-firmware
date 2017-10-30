#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <firmware_image_package.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_fip.h>
#include <io_memmap.h>
#include <io_storage.h>
#include <mmio.h>
#include <platform_def.h>

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	return 0;
}
