/* IA-16 implementation of __ashlsi3

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

	.global	__ashlsi3
	.type	__ashlsi3, @function
__ashlsi3:
#ifndef __IA16_CMODEL_IS_FAR_TEXT
# ifdef __IA16_CALLCVT_CDECL
#   ifdef __OPTIMIZE_SIZE__
	popw	%bx
	popw	%ax
	popw	%dx
	popw	%cx
	pushw	%cx
	pushw	%dx
	pushw	%ax
	pushw	%bx
#   else
	movw	%sp,	%bx
	movw	2(%bx),	%ax
	movw	4(%bx),	%dx
	movb	6(%bx),	%cl
#   endif
# elif defined __IA16_CALLCVT_STDCALL
	popw	%bx
	popw	%ax
	popw	%dx
	popw	%cx
	pushw	%bx
# endif
#else
# ifdef __IA16_CALLCVT_CDECL
	movw	%sp,	%bx
	movw	4(%bx),	%ax
	movw	6(%bx),	%dx
	movb	8(%bx),	%cl
# elif defined __IA16_CALLCVT_STDCALL
	popw	%dx
	popw	%cx
	popw	%ax
	movw	%sp,	%bx
	xchgw	%dx,	(%bx)
	xchgw	%cx,	2(%bx)
# endif
#endif
	andw	$0x1f,	%cx
# ifdef __OPTIMIZE_SIZE__
	jcxz	1f
0:
	shlw	%ax
	rclw	%dx
	loop	0b
# else
	testb	$0x10,	%cl
	jz	0f
	movw	%ax,	%dx
	xorw	%ax,	%ax
	subb	$0x10,	%cl
	shlw	%cl,	%dx
NT_(	ret	)
FT_(	lret	)
0:
	movw	%ax,	%bx
	shlw	%cl,	%ax
	/* Quickly figure out the bits we want to shift from the low
	   shortword into the high shortword.  We can do this by taking the
	   "difference" between the `shlw' and `rolw' results.  */
	rolw	%cl,	%bx
	xorw	%ax,	%bx
	/* Then combine them with the bits from the high shortword.  */
	shlw	%cl,	%dx
	orw	%bx,	%dx
#endif
1:
NT_(	ret	)
FT_(	lret	)
