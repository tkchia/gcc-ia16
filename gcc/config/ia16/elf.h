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

extern const char *rt_specs_files_spec_function (int, const char **);
extern const char *check_rt_switches_spec_function (int, const char **);
extern const char *cpp_sys_defs_spec_function (int, const char **);

#define EXTRA_SPEC_FUNCTIONS \
  { "rt-specs-files", rt_specs_files_spec_function }, \
  { "check-rt-switches", check_rt_switches_spec_function }, \
  { "cpp-sys-defs", cpp_sys_defs_spec_function },

#define DRIVER_SELF_SPECS \
  "%{mr=*:;:-mr=msdos}", \
  "%:rt-specs-files(rt-specs rt-specs%s %{mr=*:%*})", \
  "%:check-rt-switches(%(ia16_impl_rt_switches) ^ %(ia16_warn_rt_switches) " \
					       "^ %(rt_switches))", \
  "%{mdosx32:-mdosx}",	\
  "%{mdosx:"		\
    "%{march=*:;:-march=i80286} " \
    "%{mno-segelf:;:-msegelf}}", \
  "%{mr=elks:"		\
    "-nostdinc "	\
    "%{mno-segelf:;:-msegelf} " \
    "%{fuse-ld=*:;:-fuse-ld=gold} " \
    "%{mnewlib-autofloat-stdio:;:-mno-newlib-autofloat-stdio}}", \
  "%{mr=elks|mdosx:%{!mno-protected-mode:-mprotected-mode}}", \
  "%{mr=elks|mdosx|mseparate-code-segment:%{mcmodel=*:;:-mcmodel=small}}", \
  "%{mcmodel=small|mcmodel=medium:" \
    "%{!mdosx:"		\
      "%{!mno-segment-relocation-stuff:-msegment-relocation-stuff}}}"

/* 1) Define "reasonable" macros & predicates corresponding to the chosen
      target operating system.  Do this here rather than in ia16-c.c, so that
      any r-*.spec file provided by the runtime library can override our
      settings here, if they need or want to.

   2) For ELKS, make the libgcc include directories available.  This gives us
      a kind of "freestanding" build environment from which we can build
      elks-libc & install proper specs. */
#define CPP_SPEC	\
  "%{mcmodel=medium:%{,c++|,c++-header:%emedium model not supported for C++}}"\
  "%{mno-callee-assume-ss-data-segment:" \
    "%{,c++|,c++-header:%e%ss != .data not supported for C++}}" \
  "%:cpp-sys-defs(%{mr=*:%*}) " \
  "%{mr=elks:-D__ELKS__ " \
	    "-isystem include-fixed/../include%s " \
	    "-isystem include-fixed%s " \
	    "-isystem elkslibc/include%s;" \
    "mr=msdos:-D__MSDOS__}"

#define ASM_SPEC	\
  "%{msegelf:--16-segelf}"

/* Our stub built-in specs now only support the building of libgcc, but can
   no longer link complete programs.  Assume that the C library (e.g.
   Newlib or elks-libc) will provide runtime specs (r-*.spec) that say how
   to link programs.  -- tkchia 20221207 */
#define LINK_SPEC	\
  "%{!T*:%eneed runtime-specs from C library to link programs}"

#define STARTFILE_SPEC	\
  ""

#define ENDFILE_SPEC	\
  ""

/* If present, the r-*.spec runtime-specific specs file should (re)define
   %(ia16_impl_rt_switches) to cover only the runtime-specific switches that
   are recognized & handled by the r-*.spec file.  For example, it is highly
   unlikely that a non-MS-DOS target platform will be able to handle a
   -mdosx switch.

   The gcc-ia16 driver flags an error if it sees any runtime-specific
   switches that the specs file does not know about.

   The specs file can also optionally define %(ia16_warn_rt_switches), to
   cover any runtime-specific switches it recognizes & effectively ignores. */
#define EXTRA_SPECS	\
  { "ia16_impl_rt_switches", "%(rt_switches)" }, \
  { "ia16_warn_rt_switches", "" }, \
  { "rt_switches", "%{mmsdos-handle-v1} %{mnewlib-nano-stdio} " \
		   "%{mnewlib-autofloat-stdio} %{mno-newlib-autofloat-stdio} "\
		   "%{mhandle-non-i186} %{mhandle-non-i286} " \
		   "%{mdosx} %{mdosx32} " \
		   "%{maout} %{maout-total=*} %{maout-chmem=*} " \
		   "%{maout-stack=*} %{maout-heap=*} %{maout-symtab}" }
