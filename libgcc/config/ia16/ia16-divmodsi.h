/* IA-16 implementations of __udivsi3, __umodsi3, __divsi3, and __modsi3, in
   terms of internal routines

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

#ifdef L_udivsi3
	.global	__udivsi3
	.type	__udivsi3, @function
__udivsi3:
#elif defined L_umodsi3
	.global	__umodsi3
	.type	__umodsi3, @function
__umodsi3:
#elif defined L_divsi3
	.global	__divsi3
	.type	__divsi3, @function
__divsi3:
#else
	.global	__modsi3
	.type	__modsi3, @function
__modsi3:
#endif
#ifndef __IA16_CMODEL_IS_FAR_TEXT
# if defined __IA16_CALLCVT_CDECL
	movw	%sp,	%bx
	movw	2(%bx),	%ax
	movw	4(%bx),	%dx
	movw	8(%bx),	%cx
	movw	6(%bx),	%bx
# elif defined __IA16_CALLCVT_STDCALL
	/* Pop arguments but "push" the return address back.  */
	popw	%cx
	movw	%sp,	%bx
	xchgw	%cx,	6(%bx)
	popw	%ax
	popw	%dx
	popw	%bx
# elif defined __IA16_CALLCVT_REGPARMCALL
	popw	%cx
	movw	%sp,	%bx
	xchgw	%cx,	2(%bx)
	popw	%bx
# else
#   error "unknown calling convention!"
# endif
	/* Then invoke our helper routine.  */
# ifdef L_udivsi3
	jmp	__ia16_ldivmodu
# elif defined L_umodsi3
	call	__ia16_ldivmodu
	movw	%cx,	%dx
	movw	%bx,	%ax
	ret
# elif defined L_divsi3
	jmp	__ia16_ldivmods
# else
	call	__ia16_ldivmods
	movw	%cx,	%dx
	movw	%bx,	%ax
	ret
# endif
#else
# if defined __IA16_CALLCVT_CDECL || defined __IA16_CALLCVT_STDCALL
	movw	%sp,	%bx
	movw	4(%bx),	%ax
	movw	6(%bx),	%dx
	movw	10(%bx), %cx
	movw	8(%bx),	%bx
# elif defined __IA16_CALLCVT_REGPARMCALL
	movw	%sp,	%bx
	movw	6(%bx), %cx
	movw	4(%bx),	%bx
# else
#   error "unknown calling convention!"
# endif
# ifdef L_udivsi3
	call	__ia16_ldivmodu
# elif defined L_umodsi3
	call	__ia16_ldivmodu
	movw	%cx,	%dx
	movw	%bx,	%ax
# elif defined L_divsi3
	call	__ia16_ldivmods
# else
	call	__ia16_ldivmods
	movw	%cx,	%dx
	movw	%bx,	%ax
# endif
# ifdef __IA16_CALLCVT_CDECL
	lret
# elif defined __IA16_CALLCVT_STDCALL
	lret	$8
# else
	lret	$4
# endif
#endif
