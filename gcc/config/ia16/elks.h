/* GNU Compiler Collection target macros for ia16-*-elks targets.
   Copyright (C) 2007 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>

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

#define LINK_SPEC	\
  ""

#define LIB_SPEC        \
  "-lc -lelks -lnosys "	\
  "%{mseparate-code-segment:-T elks-separate.ld%s;:-T elks-combined.ld%s}"

#define STARTFILE_SPEC  \
  "crt0%O%s"

#define ENDFILE_SPEC    \
  ""

/* Run-time Target Specification */
#define TARGET_OS_CPP_BUILTINS()	\
	do { builtin_define_std ("ELKS"); } while (0)

#undef TARGET_OBJFMT_CPP_BUILTINS

/* Macros Controlling Initialization Routines */
/* We don't have .init/.fini support (yet).  */
#undef  INIT_SECTION_ASM_OP

/* Output of Assembler Instructions */
#undef  USER_LABEL_PREFIX
#define USER_LABEL_PREFIX "_"
