/* GNU Compiler Collection target macros for ia16-*-elf targets.
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

#define STARTFILE_SPEC	\
  ""

#define ENDFILE_SPEC	\
  ""

#define LIB_SPEC	\
  ""

/* Macros Controlling Initialization Routines */
/* We don't have .init/.fini support (yet).  */
#undef  INIT_SECTION_ASM_OP
