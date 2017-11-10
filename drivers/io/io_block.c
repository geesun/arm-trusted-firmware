/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <io_block.h>
#include <io_driver.h>
#include <io_storage.h>
#include <platform_def.h>
#include <string.h>
#include <utils.h>

typedef struct {
	io_block_dev_spec_t	*dev_spec;
	uintptr_t		base;
	size_t			file_pos;
	size_t			size;
} block_dev_state_t;

#define is_power_of_2(x)	((x != 0) && ((x & (x - 1)) == 0))

io_type_t device_type_block(void);

static int block_open(io_dev_info_t *dev_info, const uintptr_t spec,
		      io_entity_t *entity);
static int block_seek(io_entity_t *entity, int mode, ssize_t offset);
static int block_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		      size_t *length_read);
static int block_write(io_entity_t *entity, const uintptr_t buffer,
		       size_t length, size_t *length_written);
static int block_close(io_entity_t *entity);
static int block_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info);
static int block_dev_close(io_dev_info_t *dev_info);

static const io_dev_connector_t block_dev_connector = {
	.dev_open	= block_dev_open
};

static const io_dev_funcs_t block_dev_funcs = {
	.type		= device_type_block,
	.open		= block_open,
	.seek		= block_seek,
	.size		= NULL,
	.read		= block_read,
	.write		= block_write,
	.close		= block_close,
	.dev_init	= NULL,
	.dev_close	= block_dev_close,
};

static block_dev_state_t state_pool[MAX_IO_BLOCK_DEVICES];
static io_dev_info_t dev_info_pool[MAX_IO_BLOCK_DEVICES];

/* Track number of allocated block state */
static unsigned int block_dev_count;

io_type_t device_type_block(void)
{
	return IO_TYPE_BLOCK;
}

/* Locate a block state in the pool, specified by address */
static int find_first_block_state(const io_block_dev_spec_t *dev_spec,
				  unsigned int *index_out)
{
	int result = -ENOENT;
	for (int index = 0; index < MAX_IO_BLOCK_DEVICES; ++index) {
		/* dev_spec is used as identifier since it's unique */
		if (state_pool[index].dev_spec == dev_spec) {
			result = 0;
			*index_out = index;
			break;
		}
	}
	return result;
}

/* Allocate a device info from the pool and return a pointer to it */
static int allocate_dev_info(io_dev_info_t **dev_info)
{
	int result = -ENOMEM;
	assert(dev_info != NULL);

	if (block_dev_count < MAX_IO_BLOCK_DEVICES) {
		unsigned int index = 0;
		result = find_first_block_state(NULL, &index);
		assert(result == 0);
		/* initialize dev_info */
		dev_info_pool[index].funcs = &block_dev_funcs;
		dev_info_pool[index].info = (uintptr_t)&state_pool[index];
		*dev_info = &dev_info_pool[index];
		++block_dev_count;
	}

	return result;
}


/* Release a device info to the pool */
static int free_dev_info(io_dev_info_t *dev_info)
{
	int result;
	unsigned int index = 0;
	block_dev_state_t *state;
	assert(dev_info != NULL);

	state = (block_dev_state_t *)dev_info->info;
	result = find_first_block_state(state->dev_spec, &index);
	if (result ==  0) {
		/* free if device info is valid */
		zeromem(state, sizeof(block_dev_state_t));
		zeromem(dev_info, sizeof(io_dev_info_t));
		--block_dev_count;
	}

	return result;
}

static int block_open(io_dev_info_t *dev_info, const uintptr_t spec,
		      io_entity_t *entity)
{
	block_dev_state_t *cur;
	io_block_spec_t *region;

	assert((dev_info->info != (uintptr_t)NULL) &&
	       (spec != (uintptr_t)NULL) &&
	       (entity->info == (uintptr_t)NULL));

	region = (io_block_spec_t *)spec;
	cur = (block_dev_state_t *)dev_info->info;
	assert(((region->offset % cur->dev_spec->block_size) == 0) &&
	       ((region->length % cur->dev_spec->block_size) == 0));

	cur->base = region->offset;
	cur->size = region->length;
	cur->file_pos = 0;

	entity->info = (uintptr_t)cur;
	return 0;
}

/* parameter offset is relative address at here */
static int block_seek(io_entity_t *entity, int mode, ssize_t offset)
{
	block_dev_state_t *cur;

	assert(entity->info != (uintptr_t)NULL);

	cur = (block_dev_state_t *)entity->info;
	assert((offset >= 0) && (offset < cur->size));

	switch (mode) {
	case IO_SEEK_SET:
		cur->file_pos = offset;
		break;
	case IO_SEEK_CUR:
		cur->file_pos += offset;
		break;
	default:
		return -EINVAL;
	}
	assert(cur->file_pos < cur->size);
	return 0;
}

static int block_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		      size_t *length_read)
{
	block_dev_state_t *cur;
	io_block_spec_t *buf;
	io_block_ops_t *ops;
	size_t aligned_length, skip, count, padding, block_size;
	size_t total_read_len, read_len;
	uintptr_t p_buf;
	int lba, left;
	int buffer_not_aligned;

	assert(entity->info != (uintptr_t)NULL);
	cur = (block_dev_state_t *)entity->info;
	ops = &(cur->dev_spec->ops);
	buf = &(cur->dev_spec->buffer);
	block_size = cur->dev_spec->block_size;
	assert((length <= cur->size) &&
	       (length > 0) &&
	       (ops->read != 0));

	assert((buf->length >= block_size) &&
			(buf->length%block_size == 0));

	skip = cur->file_pos % block_size;
	aligned_length = ((skip + length) + (block_size - 1)) &
			 ~(block_size - 1);
	padding = aligned_length - (skip + length);
	left = aligned_length;
	total_read_len = 0;

	/* Read buf->length data from device every time */
	for (left = aligned_length; left > 0; left -= count) {
		lba = (cur->file_pos + cur->base) / block_size;
		read_len = (left > buf->length) ? buf->length : left;

		if (((buffer + total_read_len) & (block_size - 1)) != 0) {
			/*
			 * buffer isn't aligned with block size.
			 * Block device always relies on DMA operation.
			 * It's better to make the buffer as block size aligned.
			 */
			buffer_not_aligned = 1;
		} else {
			buffer_not_aligned = 0;
		}

		if (skip || buffer_not_aligned
				|| (left <= buf->length && padding)) {
			count = ops->read(lba, buf->offset, buf->length);
			assert(count == buf->length);
			p_buf = buf->offset;

			/* First part, shift the start point of the skip part */
			if (skip) {
				read_len -= skip;
				p_buf += skip;
			}
			/* Reach the last part, skip copy the padding */
			if (left <= buf->length && padding) {
				read_len -= padding;
			}

			memcpy((void *)buffer + total_read_len,
					(void *)(p_buf),
					read_len);
		} else {
			count = ops->read(lba, buffer + total_read_len, buf->length);
			assert(count == buf->length);
		}

		skip = 0;
		total_read_len += read_len;
		cur->file_pos += read_len;

	}

	*length_read = total_read_len;
	return 0;
}


static int block_write(io_entity_t *entity, const uintptr_t buffer,
		       size_t length, size_t *length_written)
{
	block_dev_state_t *cur;
	io_block_spec_t *buf;
	io_block_ops_t *ops;
	size_t aligned_length, skip, count, padding, block_size;
	size_t total_write_len, write_len;
	uintptr_t p_buf;
	int lba, left;
	int buffer_not_aligned;

	assert(entity->info != (uintptr_t)NULL);
	cur = (block_dev_state_t *)entity->info;
	ops = &(cur->dev_spec->ops);
	buf = &(cur->dev_spec->buffer);
	block_size = cur->dev_spec->block_size;
	assert((length <= cur->size) &&
	       (length > 0) &&
	       (ops->read != 0) &&
	       (ops->write != 0));

	assert((buf->length >= block_size) &&
			(buf->length%block_size == 0));

	skip = cur->file_pos % block_size;
	aligned_length = ((skip + length) + (block_size - 1)) &
			 ~(block_size - 1);
	padding = aligned_length - (skip + length);
	left = aligned_length;
	total_write_len = 0;

	for (left = aligned_length; left > 0; left -= count) {
		lba = (cur->file_pos + cur->base) / block_size;
		write_len = (left > buf->length) ? buf->length : left;

		if (((buffer + total_write_len) & (block_size - 1)) != 0) {
			/*
			 * buffer isn't aligned with block size.
			 * Block device always relies on DMA operation.
			 * It's better to make the buffer as block size aligned.
			 */
			buffer_not_aligned = 1;
		} else {
			buffer_not_aligned = 0;
		}

		if (skip || buffer_not_aligned
				|| (left <= buf->length && padding)) {
			count = ops->read(lba, buf->offset,
					buf->length);
			assert(count == buf->length);

			p_buf = buf->offset;
			/* First part, shift the start point of the skip part */
			if (skip) {
				write_len -= skip;
				p_buf += skip;
			}

			/* Reach the last part, skip copy the padding */
			if (left <= buf->length && padding) {
				write_len -= padding;
			}
			memcpy((void *)(p_buf),
					(void *)buffer + total_write_len,
					write_len);
			count = ops->write(lba, buf->offset,
					buf->length);
		} else {
			count = ops->write(lba, buffer + total_write_len, buf->length);
		}
		assert(count == buf->length);

		skip = 0;
		total_write_len += write_len;
		cur->file_pos += write_len;
	}

	*length_written = total_write_len;
	return 0;
}

static int block_close(io_entity_t *entity)
{
	entity->info = (uintptr_t)NULL;
	return 0;
}

static int block_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info)
{
	block_dev_state_t *cur;
	io_block_spec_t *buffer;
	io_dev_info_t *info;
	size_t block_size;
	int result;

	assert(dev_info != NULL);
	result = allocate_dev_info(&info);
	if (result)
		return -ENOENT;

	cur = (block_dev_state_t *)info->info;
	/* dev_spec is type of io_block_dev_spec_t. */
	cur->dev_spec = (io_block_dev_spec_t *)dev_spec;
	buffer = &(cur->dev_spec->buffer);
	block_size = cur->dev_spec->block_size;
	assert((block_size > 0) &&
	       (is_power_of_2(block_size) != 0) &&
	       ((buffer->offset % block_size) == 0) &&
	       ((buffer->length % block_size) == 0));

	*dev_info = info;	/* cast away const */
	(void)block_size;
	(void)buffer;
	return 0;
}

static int block_dev_close(io_dev_info_t *dev_info)
{
	return free_dev_info(dev_info);
}

/* Exported functions */

/* Register the Block driver with the IO abstraction */
int register_io_dev_block(const io_dev_connector_t **dev_con)
{
	int result;

	assert(dev_con != NULL);

	/*
	 * Since dev_info isn't really used in io_register_device, always
	 * use the same device info at here instead.
	 */
	result = io_register_device(&dev_info_pool[0]);
	if (result == 0)
		*dev_con = &block_dev_connector;
	return result;
}
