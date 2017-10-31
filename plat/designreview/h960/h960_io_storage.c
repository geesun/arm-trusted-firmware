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
#include <ufs.h>
#include <platform_def.h>



struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const io_dev_connector_t * ufs_dev_con,*memmap_dev_con, *fip_dev_con;
static uintptr_t ufs_dev_handle,memmap_dev_handle , fip_dev_handle;

static int open_memmap(const uintptr_t spec);
static int open_fip(const uintptr_t spec);

static const io_block_spec_t mmap_fip_spec = {
	.offset		= H960_FIP_BASE,
	.length		= H960_FIP_MAX_SIZE,
};

static const io_block_spec_t ufs_fip_spec = {
	.offset		= H960_UFS_FIP_BASE,
	.length		= H960_UFS_FIP_MAX_SIZE,
};

static const io_block_spec_t ufs_data_spec = {
	.offset		= 0,
	.length		= 256 << 20,
};

size_t ufs_read_lun3_blks(int lba, uintptr_t buf, size_t size)
{
	return ufs_read_blocks(3, lba, buf, size);
}

size_t ufs_write_lun3_blks(int lba, const uintptr_t buf, size_t size)
{
	return ufs_write_blocks(3, lba, buf, size);
}

static const io_block_dev_spec_t ufs_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= H960_UFS_DATA_BASE,
		.length	= H960_UFS_DATA_SIZE,
	},
	.ops		= {
		.read	= ufs_read_lun3_blks,
		.write	= ufs_write_lun3_blks,
	},
	.block_size	= UFS_BLOCK_SIZE,
};

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t scp_bl2_uuid_spec = {
	.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl32_extra1_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
};

static const io_uuid_spec_t bl32_extra2_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA2,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&mmap_fip_spec,
		open_memmap
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		open_fip
	},
	[SCP_BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_bl2_uuid_spec,
		open_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		open_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		open_fip
	},
	[BL32_EXTRA1_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_extra1_uuid_spec,
		open_fip
	},
	[BL32_EXTRA2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_extra2_uuid_spec,
		open_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		open_fip
	},
	[BL2U_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&ufs_data_spec,
		open_memmap
	}
};

static int open_memmap(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(memmap_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(memmap_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Memmap\n");
			io_close(local_image_handle);
		}
	}
	return result;
}

static int open_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			NOTICE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}


int open_ufs(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(ufs_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(ufs_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}

void hi960_io_setup(void)
{
	int result;

	result = register_io_dev_block(&ufs_dev_con);
	assert(result == 0);

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	result = register_io_dev_memmap(&memmap_dev_con);
	assert(result == 0);

	result = io_dev_open(ufs_dev_con, (uintptr_t)&ufs_dev_spec,
			     &ufs_dev_handle);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
				&memmap_dev_handle);
	assert(result == 0);
	/* Ignore improbable errors in release builds */
	(void)result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	if(result == 0){
		*image_spec = policy->image_spec;
		*dev_handle = *(policy->dev_handle);
	}else{
		NOTICE("Trying alternative IO image_id = %d \n",image_id);
		policies[FIP_IMAGE_ID].check = &open_ufs;
		policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&ufs_fip_spec;
		policies[FIP_IMAGE_ID].dev_handle = &ufs_dev_handle;
		policy = &policies[image_id];

		result = policy->check(policy->image_spec);
		if(result == 0){
			*image_spec = policy->image_spec;
			*dev_handle = *(policy->dev_handle);
		}
	}

	return result;
}

