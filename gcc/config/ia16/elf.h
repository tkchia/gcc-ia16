/* GNU Compiler Collection target macros for ia16-*-elf targets.
   Copyright (C) 2007-2017 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>
   Changes by Andrew Jenner <andrew@codesourcery.com>
   Very preliminary small code model support by TK Chia

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation; either version 3 of the License, or (at your option) any
   later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Controlling the Compilation Driver, gcc.  */

#define DRIVER_SELF_SPECS \
  "%{melks-libc:"	\
    "-nostdinc "	\
    "%{mno-segelf:;:-msegelf} " \
    "%{fuse-ld=*:;:-fuse-ld=gold}}", \
  "%{melks-libc|mdpmiable:%{!mno-protected-mode:-mprotected-mode}}", \
  "%{melks-libc|mseparate-code-segment:%{mcmodel=*:;:-mcmodel=small}}", \
  "%{mcmodel=small|mcmodel=medium:" \
    "%{!mno-segment-relocation-stuff:-msegment-relocation-stuff}}", \
  "%{maout-total=*:"	\
    "%{maout-chmem=*:%emay not use both -maout-total= and -maout-chmem=}" \
    "%{maout-stack=*:%emay not use both -maout-total= and -maout-stack=}" \
    "%{maout-heap=*:%emay not use both -maout-total= and -maout-heap=}}", \
  "%{maout-chmem=*:"	\
    "%{maout-stack=*:%emay not use both -maout-chmem= and -maout-stack=}" \
    "%{maout-heap=*:%emay not use both -maout-chmem= and -maout-heap=}}", \
  "%{mhandle-non-i186:"	\
    "%{melks-libc:%e-mhandle-non-i186 not supported for ELKS}}"

/* This is a hack.  When -melks-libc is specified, then, combined with the
   -nostdinc above, this hack will (try to) make GCC use the include files
   under the -melks-libc multilib directory, rather than the Newlib include
   files in the usual locations.

   We also need to extend the hack to
     * rope in the libgcc include directories --- via `include-fixed' ---
       since elks-libc's headers use libgcc's.
     * fall back on the include directories for the default calling
       convention (.../ia16-elf/lib/elkslibc/include/), in case the user
       says e.g. `-melks-libc -mregparmcall' and no include files are
       installed specifically for this calling convention.

   Again, this is a hack.  -- tkchia  */
#define CPP_SPEC	\
  "%{melks-libc:-isystem include-fixed/../include%s " \
	       "-isystem include-fixed%s " \
	       "-isystem include%s " \
	       "-isystem elkslibc/include%s}" \
  "%{mcmodel=medium:%{,c++|,c++-header:%emedium model not supported for C++}}"

#define ASM_SPEC	\
  "%{msegelf:--32-segelf}"

/* For -nostdlib, -nodefaultlibs, and -nostartfiles:  arrange for the linker
   script specification (%T...) to appear in LIB_SPEC if we are linking in
   default libraries, but in LINK_SPEC if we are not (i.e. -nostdlib or
   -nodefaultlibs).

   If default libraries are used, then we want them to appear at the end of
   the ld command line, so LIB_SPEC is the right place to use.

   If default libraries are not used, then GCC will not use LIB_SPEC at all,
   so we need to rely on LINK_SPEC to pass the linker script to ld.

   As for the (undocumented) -r option:  I currently just arrange not to
   pass any linker script at all if -r appears.  (FIXME: This only really
   works if something like -nostdlib is also specified.  One is out of luck
   if they would also like to link in startup files or default libraries
   during -r.)  -- tkchia  */
#define LINK_SPEC	\
  "%{!T*:"		\
    "%{mmsdos-handle-v1:-ldosv1} " \
    "%{!r:"		\
      "%{melks-libc:"	\
	  "%Telks-%(cmodel_long_ld);" \
	"nostdlib|nodefaultlibs:" \
	"%{mtsr:"	\
	    "%{nostdlib|nostartfiles:%Tdtr-m%(cmodel_ld);" \
	      ":%Tdtr-m%(cmodel_s_ld)};" \
	  "mdpmiable:"	\
	    "%{nostdlib|nostartfiles:%Tdpm-m%(cmodel_ld);" \
	      ":%Tdpm-m%(cmodel_s_ld)};" \
	  "nostdlib|nostartfiles:" \
	    "%Tdos-m%(cmodel_ld);" \
	  ":%Tdos-m%(cmodel_s_ld)" \
	"}"		\
      "} "		\
      "%{!mno-post-link:--oformat elf32-i386}" /* XXX temporary hack */ \
    "}"			\
  "} "			\
  "%{melks-libc:"	\
    "-m i386elks "	\
    "%{maout-total=*:--defsym=_total=%*} " \
    "%{maout-chmem=*:--defsym=_chmem=%*} " \
    "%{maout-stack=*:--defsym=_stack=%*} " \
    "%{maout-heap=*:--defsym=_heap=%*}} " \
  "%{!melks-libc:"	\
    "%{maout-heap=*:--defsym=__heaplen_val=%*}}"

#define STARTFILE_SPEC	\
  "%{melks-libc:-l:crt0.o}"

#define ENDFILE_SPEC	\
  ""

#define LIB_SPEC	\
  "%{melks-libc:-lc -lgcc;" \
    "!T*:"		\
    "%{!r:"		\
      "%{!nostdlib:"	\
	"%{!nodefaultlibs:" \
	  "%{mhandle-non-i186:" \
	    "%{march=any|march=i8086|march=i8088:;" \
	      "march=*:-lck086}} " \
	  "%{mnewlib-nano-stdio:" \
	    "%{mnewlib-autofloat-stdio:-lanstdio} -lnstdio;" \
	    "mnewlib-autofloat-stdio:-lastdio} " \
	  "%{mtsr:" \
	      "%{nostartfiles:%Tdtr-m%(cmodel_l_ld);:%Tdtr-m%(cmodel_sl_ld)};"\
	    "mdpmiable:" \
	      "%{nostartfiles:%Tdpm-m%(cmodel_l_ld);:%Tdpm-m%(cmodel_sl_ld)};"\
	    "nostartfiles:" \
	      "%Tdos-m%(cmodel_l_ld);" \
	    ":%Tdos-m%(cmodel_sl_ld)" \
	  "}"		\
	"}"		\
      "}"		\
    "}"			\
  "}"

#define EXTRA_SPECS	\
  { "cmodel_ld", "%{mcmodel=medium:m.ld;mcmodel=small:s.ld;:t.ld}" }, \
  { "cmodel_s_ld", "%{mcmodel=medium:ms.ld;mcmodel=small:ss.ld;:ts.ld}" }, \
  { "cmodel_l_ld", "%{mcmodel=medium:ml.ld;mcmodel=small:sl.ld;:tl.ld}" }, \
  { "cmodel_sl_ld", "%{mcmodel=medium:msl.ld;mcmodel=small:ssl.ld;:tsl.ld}" },\
  { "cmodel_long_ld", "%{mcmodel=*:%*.ld;:tiny.ld}" }

#ifndef HAVE_DOS_BASED_FILE_SYSTEM
# define POST_LINK_SPEC_OBJCOPY "ia16-elf-objcopy"
#else
# define POST_LINK_SPEC_OBJCOPY "i16objco"
#endif

#define POST_LINK_SPEC	\
  "%{!mno-post-link:"	\
    "%{melks-libc:"	\
      "elf2elks %{v} "	\
	       "%{mcmodel=tiny:--tiny} " \
	       "%{maout-total=*:--total-data %*} " \
	       "%{maout-chmem=*:--chmem %*} " \
	       "%{maout-stack=*:--stack %*} " \
	       "%{maout-heap=*:--heap %*} " \
	       "%{o*:%*} %{!o*:a.out};" \
      ":" POST_LINK_SPEC_OBJCOPY " -O binary %{o*:%*} %{!o*:a.out}}}"
