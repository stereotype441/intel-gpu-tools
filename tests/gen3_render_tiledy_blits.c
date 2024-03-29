/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

/** @file gen3_linear_render_blits.c
 *
 * This is a test of doing many blits, with a working set
 * larger than the aperture size.
 *
 * The goal is to simply ensure the basics work.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "drm.h"
#include "i915_drm.h"
#include "drmtest.h"
#include "intel_gpu_tools.h"

#include "i915_reg.h"
#include "i915_3d.h"

#define WIDTH 512
#define HEIGHT 512

static inline uint32_t pack_float(float f)
{
	union {
		uint32_t dw;
		float f;
	} u;
	u.f = f;
	return u.dw;
}

static uint32_t gem_create(int fd, int size)
{
	struct drm_i915_gem_create create;

	create.handle = 0;
	create.size = size;
	(void)drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	assert(create.handle);

	return create.handle;
}

static void gem_close(int fd, uint32_t handle)
{
	struct drm_gem_close close;
	int ret;

	close.handle = handle;
	ret = drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close);
	assert(ret == 0);
}

static uint64_t
gem_aperture_size(int fd)
{
	struct drm_i915_gem_get_aperture aperture;

	aperture.aper_size = 512*1024*1024;
	(void)drmIoctl(fd, DRM_IOCTL_I915_GEM_GET_APERTURE, &aperture);
	return aperture.aper_size;
}

static void
gem_write(int fd, uint32_t handle, int offset, int size, const void *buf)
{
	struct drm_i915_gem_pwrite pwrite;
	int ret;

	pwrite.handle = handle;
	pwrite.offset = offset;
	pwrite.size = size;
	pwrite.data_ptr = (uintptr_t)buf;
	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite);
	assert(ret == 0);
}

static uint32_t fill_reloc(struct drm_i915_gem_relocation_entry *reloc,
			   uint32_t offset,
			   uint32_t handle,
			   uint32_t read_domain,
			   uint32_t write_domain)
{
	reloc->target_handle = handle;
	reloc->delta = 0;
	reloc->offset = offset * sizeof(uint32_t);
	reloc->presumed_offset = 0;
	reloc->read_domains = read_domain;
	reloc->write_domain = write_domain;

	return reloc->presumed_offset + reloc->delta;
}

static void
copy(int fd, uint32_t dst, uint32_t src)
{
	uint32_t batch[1024], *b = batch;
	struct drm_i915_gem_relocation_entry reloc[2], *r = reloc;
	struct drm_i915_gem_exec_object2 obj[3];
	struct drm_i915_gem_execbuffer2 exec;
	uint32_t handle;
	int ret;

	/* invariant state */
	*b++ = (_3DSTATE_AA_CMD |
		AA_LINE_ECAAR_WIDTH_ENABLE |
		AA_LINE_ECAAR_WIDTH_1_0 |
		AA_LINE_REGION_WIDTH_ENABLE | AA_LINE_REGION_WIDTH_1_0);
	*b++ = (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD |
		IAB_MODIFY_ENABLE |
		IAB_MODIFY_FUNC | (BLENDFUNC_ADD << IAB_FUNC_SHIFT) |
		IAB_MODIFY_SRC_FACTOR | (BLENDFACT_ONE <<
					 IAB_SRC_FACTOR_SHIFT) |
		IAB_MODIFY_DST_FACTOR | (BLENDFACT_ZERO <<
					 IAB_DST_FACTOR_SHIFT));
	*b++ = (_3DSTATE_DFLT_DIFFUSE_CMD);
	*b++ = (0);
	*b++ = (_3DSTATE_DFLT_SPEC_CMD);
	*b++ = (0);
	*b++ = (_3DSTATE_DFLT_Z_CMD);
	*b++ = (0);
	*b++ = (_3DSTATE_COORD_SET_BINDINGS |
		CSB_TCB(0, 0) |
		CSB_TCB(1, 1) |
		CSB_TCB(2, 2) |
		CSB_TCB(3, 3) |
		CSB_TCB(4, 4) |
		CSB_TCB(5, 5) | CSB_TCB(6, 6) | CSB_TCB(7, 7));
	*b++ = (_3DSTATE_RASTER_RULES_CMD |
		ENABLE_POINT_RASTER_RULE |
		OGL_POINT_RASTER_RULE |
		ENABLE_LINE_STRIP_PROVOKE_VRTX |
		ENABLE_TRI_FAN_PROVOKE_VRTX |
		LINE_STRIP_PROVOKE_VRTX(1) |
		TRI_FAN_PROVOKE_VRTX(2) | ENABLE_TEXKILL_3D_4D | TEXKILL_4D);
	*b++ = (_3DSTATE_MODES_4_CMD |
		ENABLE_LOGIC_OP_FUNC | LOGIC_OP_FUNC(LOGICOP_COPY) |
		ENABLE_STENCIL_WRITE_MASK | STENCIL_WRITE_MASK(0xff) |
		ENABLE_STENCIL_TEST_MASK | STENCIL_TEST_MASK(0xff));
	*b++ = (_3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(3) | I1_LOAD_S(4) | I1_LOAD_S(5) | 2);
	*b++ = (0x00000000);	/* Disable texture coordinate wrap-shortest */
	*b++ = ((1 << S4_POINT_WIDTH_SHIFT) |
		S4_LINE_WIDTH_ONE |
		S4_CULLMODE_NONE |
		S4_VFMT_XY);
	*b++ = (0x00000000);	/* Stencil. */
	*b++ = (_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);
	*b++ = (_3DSTATE_SCISSOR_RECT_0_CMD);
	*b++ = (0);
	*b++ = (0);
	*b++ = (_3DSTATE_DEPTH_SUBRECT_DISABLE);
	*b++ = (_3DSTATE_LOAD_INDIRECT | 0);	/* disable indirect state */
	*b++ = (0);
	*b++ = (_3DSTATE_STIPPLE);
	*b++ = (0x00000000);
	*b++ = (_3DSTATE_BACKFACE_STENCIL_OPS | BFO_ENABLE_STENCIL_TWO_SIDE | 0);

	/* samler state */
#define TEX_COUNT 1
	*b++ = (_3DSTATE_MAP_STATE | (3 * TEX_COUNT));
	*b++ = ((1 << TEX_COUNT) - 1);
	*b = fill_reloc(r++, b-batch, src, I915_GEM_DOMAIN_SAMPLER, 0); b++;
	*b++ = (MAPSURF_32BIT | MT_32BIT_ARGB8888 |
		MS3_TILED_SURFACE | MS3_TILE_WALK |
		(HEIGHT - 1) << MS3_HEIGHT_SHIFT |
		(WIDTH - 1) << MS3_WIDTH_SHIFT);
	*b++ = ((WIDTH-1) << MS4_PITCH_SHIFT);

	*b++ = (_3DSTATE_SAMPLER_STATE | (3 * TEX_COUNT));
	*b++ = ((1 << TEX_COUNT) - 1);
	*b++ = (MIPFILTER_NONE << SS2_MIP_FILTER_SHIFT |
		FILTER_NEAREST << SS2_MAG_FILTER_SHIFT |
		FILTER_NEAREST << SS2_MIN_FILTER_SHIFT);
	*b++ = (TEXCOORDMODE_WRAP << SS3_TCX_ADDR_MODE_SHIFT |
		TEXCOORDMODE_WRAP << SS3_TCY_ADDR_MODE_SHIFT |
		0 << SS3_TEXTUREMAP_INDEX_SHIFT);
	*b++ = (0x00000000);

	/* render target state */
	*b++ = (_3DSTATE_BUF_INFO_CMD);
	*b++ = (BUF_3D_ID_COLOR_BACK | BUF_3D_TILED_SURFACE | BUF_3D_TILE_WALK_Y | WIDTH*4);
	*b = fill_reloc(r++, b-batch, dst,
			I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
	b++;

	*b++ = (_3DSTATE_DST_BUF_VARS_CMD);
	*b++ = (COLR_BUF_ARGB8888 |
		DSTORG_HORT_BIAS(0x8) |
		DSTORG_VERT_BIAS(0x8));

	/* draw rect is unconditional */
	*b++ = (_3DSTATE_DRAW_RECT_CMD);
	*b++ = (0x00000000);
	*b++ = (0x00000000);	/* ymin, xmin */
	*b++ = (DRAW_YMAX(HEIGHT - 1) |
		DRAW_XMAX(WIDTH - 1));
	/* yorig, xorig (relate to color buffer?) */
	*b++ = (0x00000000);

	/* texfmt */
	*b++ = (_3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(1) | I1_LOAD_S(2) | I1_LOAD_S(6) | 2);
	*b++ = ((4 << S1_VERTEX_WIDTH_SHIFT) | (4 << S1_VERTEX_PITCH_SHIFT));
	*b++ = (~S2_TEXCOORD_FMT(0, TEXCOORDFMT_NOT_PRESENT) |
		S2_TEXCOORD_FMT(0, TEXCOORDFMT_2D));
	*b++ = (S6_CBUF_BLEND_ENABLE | S6_COLOR_WRITE_ENABLE |
		BLENDFUNC_ADD << S6_CBUF_BLEND_FUNC_SHIFT |
		BLENDFACT_ONE << S6_CBUF_SRC_BLEND_FACT_SHIFT |
		BLENDFACT_ZERO << S6_CBUF_DST_BLEND_FACT_SHIFT);

	/* pixel shader */
	*b++ = (_3DSTATE_PIXEL_SHADER_PROGRAM | (1 + 3*3 - 2));
	/* decl FS_T0 */
	*b++ = (D0_DCL |
		REG_TYPE(FS_T0) << D0_TYPE_SHIFT |
		REG_NR(FS_T0) << D0_NR_SHIFT |
		((REG_TYPE(FS_T0) != REG_TYPE_S) ? D0_CHANNEL_ALL : 0));
	*b++ = (0);
	*b++ = (0);
	/* decl FS_S0 */
	*b++ = (D0_DCL |
		(REG_TYPE(FS_S0) << D0_TYPE_SHIFT) |
		(REG_NR(FS_S0) << D0_NR_SHIFT) |
		((REG_TYPE(FS_S0) != REG_TYPE_S) ? D0_CHANNEL_ALL : 0));
	*b++ = (0);
	*b++ = (0);
	/* texld(FS_OC, FS_S0, FS_T0 */
	*b++ = (T0_TEXLD |
		(REG_TYPE(FS_OC) << T0_DEST_TYPE_SHIFT) |
		(REG_NR(FS_OC) << T0_DEST_NR_SHIFT) |
		(REG_NR(FS_S0) << T0_SAMPLER_NR_SHIFT));
	*b++ = ((REG_TYPE(FS_T0) << T1_ADDRESS_REG_TYPE_SHIFT) |
		(REG_NR(FS_T0) << T1_ADDRESS_REG_NR_SHIFT));
	*b++ = (0);

	*b++ = (PRIM3D_RECTLIST | (3*4 - 1));
	*b++ = pack_float(WIDTH);
	*b++ = pack_float(HEIGHT);
	*b++ = pack_float(WIDTH);
	*b++ = pack_float(HEIGHT);

	*b++ = pack_float(0);
	*b++ = pack_float(HEIGHT);
	*b++ = pack_float(0);
	*b++ = pack_float(HEIGHT);

	*b++ = pack_float(0);
	*b++ = pack_float(0);
	*b++ = pack_float(0);
	*b++ = pack_float(0);

	*b++ = MI_BATCH_BUFFER_END;
	if ((b - batch) & 1)
		*b++ = 0;

	assert(b - batch <= 1024);
	handle = gem_create(fd, 4096);
	gem_write(fd, handle, 0, (b-batch)*sizeof(batch[0]), batch);

	assert(r-reloc == 2);

	obj[0].handle = dst;
	obj[0].relocation_count = 0;
	obj[0].relocs_ptr = 0;
	obj[0].alignment = 0;
	obj[0].offset = 0;
	obj[0].flags = 0;
	obj[0].rsvd1 = 0;
	obj[0].rsvd2 = 0;

	obj[1].handle = src;
	obj[1].relocation_count = 0;
	obj[1].relocs_ptr = 0;
	obj[1].alignment = 0;
	obj[1].offset = 0;
	obj[1].flags = 0;
	obj[1].rsvd1 = 0;
	obj[1].rsvd2 = 0;

	obj[2].handle = handle;
	obj[2].relocation_count = 2;
	obj[2].relocs_ptr = (uintptr_t)reloc;
	obj[2].alignment = 0;
	obj[2].offset = 0;
	obj[2].flags = 0;
	obj[2].rsvd1 = obj[2].rsvd2 = 0;

	exec.buffers_ptr = (uintptr_t)obj;
	exec.buffer_count = 3;
	exec.batch_start_offset = 0;
	exec.batch_len = (b-batch)*sizeof(batch[0]);
	exec.DR1 = exec.DR4 = 0;
	exec.num_cliprects = 0;
	exec.cliprects_ptr = 0;
	exec.flags = 0;
	exec.rsvd1 = exec.rsvd2 = 0;

	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &exec);
	while (ret && errno == EBUSY) {
		drmCommandNone(fd, DRM_I915_GEM_THROTTLE);
		ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &exec);
	}
	assert(ret == 0);

	gem_close(fd, handle);
}

static void *gem_mmap(int fd, uint32_t handle, int size, int prot)
{
	struct drm_i915_gem_mmap_gtt mmap_arg;
	void *ptr;

	mmap_arg.handle = handle;
	if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg)) {
		assert(0);
		return NULL;
	}

	ptr = mmap(0, size, prot, MAP_SHARED, fd, mmap_arg.offset);
	if (ptr == MAP_FAILED) {
		assert(0);
		ptr = NULL;
	}

	return ptr;
}

static void gem_set_tiling(int fd, uint32_t handle, int tiling, int stride)
{
	struct drm_i915_gem_set_tiling set_tiling;
	int ret;

	do {
		set_tiling.handle = handle;
		set_tiling.tiling_mode = tiling;
		set_tiling.stride = stride;

		ret = ioctl(fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));
	assert(ret == 0);
	assert(set_tiling.tiling_mode == tiling);
}

static uint32_t
create_bo(int fd, uint32_t val)
{
	uint32_t handle;
	uint32_t *v;
	int i;

	handle = gem_create(fd, WIDTH*HEIGHT*4);
	gem_set_tiling(fd, handle, I915_TILING_Y, WIDTH*4);

	/* Fill the BO with dwords starting at val */
	v = gem_mmap(fd, handle, WIDTH*HEIGHT*4, PROT_READ | PROT_WRITE);
	for (i = 0; i < WIDTH*HEIGHT; i++)
		v[i] = val++;
	munmap(v, WIDTH*HEIGHT*4);

	return handle;
}

static void
check_bo(int fd, uint32_t handle, uint32_t val)
{
	uint32_t *v;
	int i;

	v = gem_mmap(fd, handle, WIDTH*HEIGHT*4, PROT_READ);
	for (i = 0; i < WIDTH*HEIGHT; i++) {
		if (v[i] != val) {
			fprintf(stderr, "Expected 0x%08x, found 0x%08x "
				"at offset 0x%08x\n",
				val, v[i], i * 4);
			abort();
		}
		val++;
	}
	munmap(v, WIDTH*HEIGHT*4);
}

int main(int argc, char **argv)
{
	uint32_t *handle, *start_val;
	uint32_t start = 0;
	int i, fd, count;

	fd = drm_open_any();

	if (!IS_GEN3(intel_get_drm_devid(fd))) {
		printf("gen3-only test, doing nothing\n");
		return 77;
	}

	count = 0;
	if (argc > 1)
		count = atoi(argv[1]);
	if (count == 0)
		count = 3 * gem_aperture_size(fd) / (1024*1024) / 2;
	printf("Using %d 1MiB buffers\n", count);

	handle = malloc(sizeof(uint32_t)*count*2);
	start_val = handle + count;

	for (i = 0; i < count; i++) {
		handle[i] = create_bo(fd, start);
		start_val[i] = start;
		start += 1024 * 1024 / 4;
	}

	printf("Verifying initialisation..."); fflush(stdout);
	for (i = 0; i < count; i++)
		check_bo(fd, handle[i], start_val[i]);
	printf("done\n");

	printf("Cyclic blits, forward..."); fflush(stdout);
	for (i = 0; i < count * 32; i++) {
		int src = i % count;
		int dst = (i + 1) % count;

		copy(fd, handle[dst], handle[src]);
		start_val[dst] = start_val[src];
	}
	printf("verifying..."); fflush(stdout);
	for (i = 0; i < count; i++)
		check_bo(fd, handle[i], start_val[i]);
	printf("done\n");

	printf("Cyclic blits, backward..."); fflush(stdout);
	for (i = 0; i < count * 32; i++) {
		int src = (i + 1) % count;
		int dst = i % count;

		copy(fd, handle[dst], handle[src]);
		start_val[dst] = start_val[src];
	}
	printf("verifying..."); fflush(stdout);
	for (i = 0; i < count; i++)
		check_bo(fd, handle[i], start_val[i]);
	printf("done\n");

	printf("Random blits..."); fflush(stdout);
	for (i = 0; i < count * 32; i++) {
		int src = random() % count;
		int dst = random() % count;

		while (src == dst)
			dst = random() % count;

		copy(fd, handle[dst], handle[src]);
		start_val[dst] = start_val[src];
	}
	printf("verifying..."); fflush(stdout);
	for (i = 0; i < count; i++)
		check_bo(fd, handle[i], start_val[i]);
	printf("done\n");

	return 0;
}
