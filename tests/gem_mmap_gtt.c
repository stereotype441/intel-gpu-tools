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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "drm.h"
#include "i915_drm.h"
#include "drmtest.h"

#define OBJECT_SIZE (16*1024*1024)

static uint32_t gem_create(int fd, int size)
{
	struct drm_i915_gem_create create;
	int ret;

	create.handle = 0;
	create.size = (size + 4095) & -4096;
	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
	assert(ret == 0);

	return create.handle;
}

static void gem_write(int fd,
		      uint32_t handle, uint32_t offset,
		      const void *src, int length)
{
	struct drm_i915_gem_pwrite arg;
	int ret;

	arg.handle = handle;
	arg.offset = offset;
	arg.size = length;
	arg.data_ptr = (uintptr_t)src;

	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &arg);
	assert(ret == 0);
}

static void gem_read(int fd,
		     uint32_t handle, uint32_t offset,
		     void *dst, int length)
{
	struct drm_i915_gem_pread arg;
	int ret;

	arg.handle = handle;
	arg.offset = offset;
	arg.size = length;
	arg.data_ptr = (uintptr_t)dst;

	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_PREAD, &arg);
	assert(ret == 0);
}

static void gem_close(int fd, uint32_t handle)
{
	struct drm_gem_close close;
	int ret;

	close.handle = handle;
	ret = drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close);
	assert(ret == 0);
}

static void set_domain(int fd, uint32_t handle)
{
	struct drm_i915_gem_set_domain set_domain;
	int ret;

	set_domain.handle = handle;
	set_domain.read_domains = I915_GEM_DOMAIN_GTT;
	set_domain.write_domain = I915_GEM_DOMAIN_GTT;

	ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
	assert(ret == 0);
}

static void *
mmap_bo(int fd, uint32_t handle)
{
	struct drm_i915_gem_mmap_gtt arg;
	void *ptr;
	int ret;

	memset(&arg, 0, sizeof(arg));

	arg.handle = handle;
	ret = ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &arg);
	assert(ret == 0);

	ptr = mmap(0, OBJECT_SIZE, PROT_READ | PROT_WRITE,
		   MAP_SHARED, fd, arg.offset);
	assert(ptr != MAP_FAILED);

	return ptr;
}

static void *
create_pointer(int fd)
{
	uint32_t handle;
	void *ptr;

	handle = gem_create(fd, OBJECT_SIZE);

	ptr = mmap_bo(fd, handle);

	gem_close(fd, handle);

	return ptr;
}

static void
test_copy(int fd)
{
	void *src, *dst;

	/* copy from a fresh src to fresh dst to force pagefault on both */
	src = create_pointer(fd);
	dst = create_pointer(fd);

	memcpy(dst, src, OBJECT_SIZE);
	memcpy(src, dst, OBJECT_SIZE);

	munmap(dst, OBJECT_SIZE);
	munmap(src, OBJECT_SIZE);
}

static void
test_write(int fd)
{
	void *src;
	uint32_t dst;

	/* copy from a fresh src to fresh dst to force pagefault on both */
	src = create_pointer(fd);
	dst = gem_create(fd, OBJECT_SIZE);

	gem_write(fd, dst, 0, src, OBJECT_SIZE);

	gem_close(fd, dst);
	munmap(src, OBJECT_SIZE);
}

static void
test_write_gtt(int fd)
{
	uint32_t dst;
	char *dst_gtt;
	void *src;

	dst = gem_create(fd, OBJECT_SIZE);

	/* prefault object into gtt */
	dst_gtt = mmap_bo(fd, dst);
	set_domain(fd, dst);
	memset(dst_gtt, 0, OBJECT_SIZE);
	munmap(dst_gtt, OBJECT_SIZE);

	src = create_pointer(fd);

	gem_write(fd, dst, 0, src, OBJECT_SIZE);

	gem_close(fd, dst);
	munmap(src, OBJECT_SIZE);
}

static void
test_read(int fd)
{
	void *dst;
	uint32_t src;

	/* copy from a fresh src to fresh dst to force pagefault on both */
	dst = create_pointer(fd);
	src = gem_create(fd, OBJECT_SIZE);

	gem_read(fd, src, 0, dst, OBJECT_SIZE);

	gem_close(fd, src);
	munmap(dst, OBJECT_SIZE);
}

int main(int argc, char **argv)
{
	int fd;

	fd = drm_open_any();

	test_copy(fd);
	test_read(fd);
	test_write(fd);
	test_write_gtt(fd);

	close(fd);

	return 0;
}
