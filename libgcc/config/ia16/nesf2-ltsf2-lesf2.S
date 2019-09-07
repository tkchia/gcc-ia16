/* IA-16 forwarder routines for single-precision floating point comparisons

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

#include "ia16-preamble.h"

	/*
	 * It would be better if ELF supports aliases to external symbols
	 * (blog.omega-prime.co.uk/2011/07/06/the-sad-state-of-symbol-aliases/)
	 * --- then we can get rid of even the jump.  -- tkchia 20190322
	 */
	.global	__nesf2
	.global	__ltsf2
	.global	__lesf2
	.type	__nesf2, @function
	.type	__ltsf2, @function
	.type	__lesf2, @function
__nesf2:
__ltsf2:
__lesf2:
NT_(	jmp	__eqsf2			)
FT_(	SR_(.+3, __eqsf2)		)
FT_(	ljmp	$0,	$__eqsf2	)
