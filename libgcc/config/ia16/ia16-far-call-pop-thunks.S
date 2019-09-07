/* IA-16 far -> near call thunks with argument popping

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

	/* These routine must go into the near text segment, if at all.  */
	.text

	.global	__ia16_far_call_pop_thunk.bx.si.si
	.type	__ia16_far_call_pop_thunk.bx.si.si, @function
	.global	__ia16_far_call_pop_thunk.bx.di.di
	.type	__ia16_far_call_pop_thunk.bx.di.di, @function
__ia16_far_call_pop_thunk.bx.si.si:
	pushw	%si			/* remember stack arguments size */
	pushw	%bp
	movw	%sp,	%bp
	pushw	%bx			/* preserve %bx, %cx, and %ax */
	pushw	%cx
	pushw	%ax
	movw	%si,	%cx		/* move stack args. size to %cx */
	jmp	0f
__ia16_far_call_pop_thunk.bx.di.di:
	pushw	%di
	pushw	%bp
	movw	%sp,	%bp
	pushw	%bx
	pushw	%cx
	pushw	%ax
	movw	%di,	%cx
0:					/* re-push stack arguments */
	jcxz	2f
	leaw	6(%bp),	%bx
	addw	%cx,	%bx
	movw	%bx,	%ax
	shrw	%cx
1:
	pushw	%ss:(%bx)
	decw	%bx
	decw	%bx
	loop	1b
	xchgw	%ax,	%bx		/* overwrite original stack */
	movw	6(%bp),	%ax		/*   arguments with return address */
	movw	%ax,	%ss:(%bx)
	movw	4(%bp),	%ax
	movw	%ax,	%ss:-2(%bx)
2:
	movw	-6(%bp), %ax		/* reload %ax and %cx for callee */
	movw	-4(%bp), %cx
	callw	*-2(%bp)		/* call function */
	movw	%bp,	%sp		/* tear down stack frame */
	popw	%bp
	popw	%bx
	addw	%bx,	%sp		/* pop stack arguments --- except
					   for the return address we wrote */
	lretw				/* return! */
