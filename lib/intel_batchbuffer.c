/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "drm.h"
#include "intel_batchbuffer.h"
#include "intel_bufmgr.h"

void
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
	if (batch->bo != NULL) {
		drm_intel_bo_unreference(batch->bo);
		batch->bo = NULL;
	}

	if (!batch->buffer)
		batch->buffer = malloc(BATCH_SZ);

	batch->bo = drm_intel_bo_alloc(batch->bufmgr, "batchbuffer",
				       BATCH_SZ, 4096);

	batch->map = batch->buffer;
	batch->size = BATCH_SZ;
	batch->ptr = batch->map;
}

struct intel_batchbuffer *
intel_batchbuffer_alloc(drm_intel_bufmgr *bufmgr)
{
	struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

	batch->bufmgr = bufmgr;
	intel_batchbuffer_reset(batch);

	return batch;
}

void
intel_batchbuffer_free(struct intel_batchbuffer *batch)
{
	free (batch->buffer);

	drm_intel_bo_unreference(batch->bo);
	batch->bo = NULL;
	free(batch);
}

void
intel_batchbuffer_flush(struct intel_batchbuffer *batch)
{
	unsigned int used = batch->ptr - batch->map;
	int ret;

	if (used == 0)
		return;

	/* Round batchbuffer usage to 2 DWORDs. */
	if ((used & 4) == 0) {
		*(uint32_t *) (batch->ptr) = 0; /* noop */
		batch->ptr += 4;
		used = batch->ptr - batch->map;
	}

	/* Mark the end of the buffer. */
	*(uint32_t *) (batch->ptr) = MI_BATCH_BUFFER_END; /* noop */
	batch->ptr += 4;
	used = batch->ptr - batch->map;

	drm_intel_bo_subdata(batch->bo, 0, used, batch->buffer);

	batch->map = NULL;
	batch->ptr = NULL;

	ret = drm_intel_bo_exec(batch->bo, used, NULL, 0, 0);
	assert(ret == 0);

	intel_batchbuffer_reset(batch);
}


/*  This is the only way buffers get added to the validate list.
 */
void
intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                             drm_intel_bo *buffer, uint32_t delta,
			     uint32_t read_domains, uint32_t write_domain)
{
	int ret;

	if (batch->ptr - batch->map > batch->bo->size)
		printf("bad relocation ptr %p map %p offset %d size %ld\n",
		       batch->ptr, batch->map, batch->ptr - batch->map,
		       batch->bo->size);

	ret = drm_intel_bo_emit_reloc(batch->bo, batch->ptr - batch->map,
				      buffer, delta,
				      read_domains, write_domain);
	intel_batchbuffer_emit_dword(batch, buffer->offset + delta);
	assert(ret == 0);
}

void
intel_batchbuffer_data(struct intel_batchbuffer *batch,
                       const void *data, unsigned int bytes)
{
	assert((bytes & 3) == 0);
	intel_batchbuffer_require_space(batch, bytes);
	memcpy(batch->ptr, data, bytes);
	batch->ptr += bytes;
}