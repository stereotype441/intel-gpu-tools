noinst_PROGRAMS = \
	gem_stress \
	$(TESTS_progs) \
	$(HANG) \
	$(NULL)

gem_stress_SOURCES = \
	gem_stress.c \
	gem_stress.h \
	gem_stress_i915.c \
	gem_stress_i830.c \
	gen6_render.h \
	gem_stress_gen6.c \
	$(NULL)

TESTS_progs = \
	getversion \
	getclient \
	getstats \
	gem_basic \
	gem_exec_nop \
	gem_exec_blt \
	gem_flink \
	gem_readwrite \
	gem_ringfill \
	gem_mmap \
	gem_mmap_gtt \
	gem_pwrite \
	gem_pread_after_blit \
	gem_tiled_pread \
	gem_linear_blits \
	gem_vmap_blits \
	gem_tiled_blits \
	gem_tiled_fence_blits \
	gem_largeobject \
	gem_bad_length \
	gem_fence_thrash \
	gem_fenced_exec_thrash \
	gem_gtt_speed \
	gen3_render_linear_blits \
	gen3_render_tiledx_blits \
	gen3_render_tiledy_blits \
	gen3_render_mixed_blits \
	gen3_mixed_blits \
	gem_storedw_loop_render \
	gem_storedw_loop_blt \
	gem_storedw_loop_bsd \
	gem_storedw_batches_loop \
	gem_dummy_reloc_loop \
	gem_double_irq_loop \
	gem_ring_sync_loop \
	gem_pipe_control_store_loop \
	gem_hangcheck_forcewake \
	gem_unref_active_buffers \
	$(NULL)

TESTS_scripts = \
	debugfs_reader \
	debugfs_emon_crash \
	sysfs_edid_timing \
	$(NULL)

kernel_tests = \
	$(TESTS_progs) \
	$(TESTS_scripts) \
	$(NULL)

TESTS = \
	$(NULL)

test:
	whoami | grep root || ( echo ERROR: not running as root; exit 1 )
	./check_drm_clients
	make TESTS="${kernel_tests}" check

HANG = \
	gem_bad_batch \
	gem_hang \
	gem_bad_blit \
	gem_bad_address \
	$(NULL)

EXTRA_PROGRAMS = $(TESTS_progs) $(HANG)
EXTRA_DIST = $(TESTS_scripts) check_drm_clients
CLEANFILES = $(EXTRA_PROGRAMS)

AM_CFLAGS = $(DRM_CFLAGS) $(WARN_CFLAGS) \
	-I$(srcdir)/.. \
	-I$(srcdir)/../lib
LDADD = ../lib/libintel_tools.la $(PCIACCESS_LIBS) $(DRM_LIBS) 

if HAVE_CAIRO
if HAVE_LIBUDEV
if HAVE_GLIB
TESTS_progs += testdisplay
LDADD += $(CAIRO_LIBS) $(LIBUDEV_LIBS) $(GLIB_LIBS)
AM_CFLAGS += $(CAIRO_CFLAGS) $(LIBUDEV_CFLAGS) $(GLIB_CFLAGS)
endif
endif
endif

gem_fence_thrash_CFLAGS = $(AM_CFLAGS) -pthread
gem_fence_thrash_LDADD = $(LDADD) -lpthread
