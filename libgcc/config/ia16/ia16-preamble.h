/* Common preamble for IA-16 assembly modules in libgcc

   Copyright (C) 2019 Free Software Foundation, Inc.
   Written By TK Chia

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

	.arch i8086,jumps
	.code16
	.att_syntax prefix

#ifdef __IA16_CMODEL_IS_FAR_TEXT
	/*
	 * Under a far-text memory model (e.g. medium model), place all our
	 * assembly language libgcc routines in the same far text segment. 
	 * This allows the libgcc API routines to invoke internal routines
	 * using near calls.
	 */
	.section .fartext.f.libgcc2.a.LIB2ADD$, "ax", @progbits
	/*
	 * FT_ (INSN) conditionally assembles an instruction INSN iff we are
	 * building for a far-text memory model.  NT_ (INSN) does the opposite.
	 *
	 * SR_ (PLACE, SYM) installs an IA-16 segment relocation for SYM's
	 * segment, at the place PLACE.
	 */
# define FT_(insn...)		insn
# define NT_(insn...)
#else
	.text
# define FT_(insn...)
# define NT_(insn...)		insn
#endif
#ifdef __IA16_ABI_SEGELF
# define SR__(place, aux)	.reloc (place), R_386_SEG16, #aux
# define SR_(place, sym)	SR__(place, sym##!)
#else
# define SR_(place, sym)	.reloc (place), R_386_OZSEG16, sym
#endif
