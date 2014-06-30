# Plugin for offload execution on Intel MIC devices.
#
# Copyright (C) 2014 Free Software Foundation, Inc.
#
# Contributed by Ilya Verbin <ilya.verbin@intel.com>.
#
# This file is part of the GNU OpenMP Library (libgomp).
#
# Libgomp is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the GCC Runtime Library Exception, version
# 3.1, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License and
# a copy of the GCC Runtime Library Exception along with this program;
# see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
# <http://www.gnu.org/licenses/>.


ACLOCAL_AMFLAGS = -I ../../.. -I ../../../config

# Directories
build_dir=$(top_builddir)
source_dir=$(top_srcdir)
libgomp_dir=$(build_dir)/../..
liboffload_src_dir=$(top_srcdir)/../../../liboffloadmic
liboffload_host_dir=$(top_builddir)/../../../liboffloadmic
#liboffload_target_dir=$(top_builddir)/../../../liboffload/target
liboffload_target_dir=$(LIBOFFLOAD_TARGET_PATH)

# Compilers
compiler_host=$(CXX)
# FORNOW
compiler_target=$(CXX)

# Make targets
.PHONY: all plugin clean

all-local: plugin

$(build_dir):
	@mkdir -p $@
	@echo "Created $@ directory"

$(build_dir)/offload_target_main: $(source_dir)/offload_target_main.cpp | $(build_dir)
	$(compiler_target) -I$(liboffload_src_dir)/include/coi/ -I$(libgomp_dir) -I$(liboffload_src_dir)/runtime/ -DHOST_LIBRARY=0 -DLINUX -DOFFLOAD_DEBUG=1 -L$(liboffload_target_dir) -loffloadmic -rdynamic -Wl,--unresolved-symbols=ignore-in-shared-libs $(liboffload_target_dir)/ofldbegin.o $< $(liboffload_target_dir)/ofldend.o -o $@

$(build_dir)/main_target_image.h: $(build_dir)/offload_target_main | $(build_dir)
	@echo -n "const int image_size = " > $@
	@stat -c '%s' $< >> $@
	@echo ";" >> $@
	@echo "struct MainTargetImage {" >> $@
	@echo "  int64_t size;" >> $@
	@echo "  char name[sizeof \"offload_target_main\"];" >> $@
	@echo "  char data[image_size];" >> $@
	@echo "};" >> $@
	@echo "extern \"C\" const MainTargetImage main_target_image = {" >> $@
	@echo "  image_size, \"offload_target_main\"," >> $@
	@cat $< | xxd -include >> $@
	@echo "};" >> $@

$(build_dir)/libgomp-plugin-mic.so.1: $(source_dir)/libgomp-plugin-mic.cpp $(build_dir)/main_target_image.h | $(build_dir)
	$(compiler_host) -I$(build_dir) -I$(libgomp_dir) -I$(liboffload_src_dir)/include/coi/ -I$(liboffload_src_dir)/runtime/ -DHOST_LIBRARY=1 -DLINUX -DOFFLOAD_DEBUG=1 -DTIMING_SUPPORT -L$(liboffload_host_dir)/.libs -loffloadmic -fPIC -shared $< -o $@

plugin: $(build_dir)/libgomp-plugin-mic.so.1 | $(build_dir)

clean:
	@rm -rf $(build_dir)
	@echo "Remove $(build_dir) directory"

