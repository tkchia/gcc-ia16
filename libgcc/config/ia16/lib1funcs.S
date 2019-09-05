	.arch i8086,jumps
	.code16
	.att_syntax prefix

	.text

#if defined L_udivsi3 || defined L_umodsi3 \
    || defined L_divsi3 || defined L_modsi3
# ifdef L_udivsi3
	.global	__udivsi3
	.type	__udivsi3, @function
__udivsi3:
# elif defined L_umodsi3
	.global	__umodsi3
	.type	__umodsi3, @function
__umodsi3:
# elif defined L_divsi3
	.global	__divsi3
	.type	__divsi3, @function
__divsi3:
# else
	.global	__modsi3
	.type	__modsi3, @function
__modsi3:
# endif
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
#elif defined L_ia16_ldivmods
	/*
	 *	"When integers are divided, the result of the / operator is
	 *	 the algebraic quotient with any fractional part discarded."
	 *
	 *	(footnote) "This is often called `truncation towards zero.'"
	 *		-- N1570 draft of ISO/IEC 9899:2011 ("C11")
	 *
	 * CMU's SEI CERT C Coding Standard document (https://wiki.sei.cmu.edu/
	 * confluence/pages/viewpage.action?pageId=87152120) further
	 * elaborates on what this means:
	 *
	 *	"In C89 (and historical K&R implementations), the meaning of
	 *	 the remainder operator for negative operands was
	 *	 implementation-defined.  This behavior was changed in C99, and
	 *	 the change remains in C11.  ...
	 *
	 *	"The result [of the modulo operator, %] has the same sign as
	 *	 the dividend (the first operand in the expression)."
	 */
	.global	__ia16_ldivmods
	.type	__ia16_ldivmods, @function
__ia16_ldivmods:
	testw	%cx,	%cx
	js	.n_divor
	testw	%dx,	%dx
	js	.p_divor_n_divnd
	/* Both divisor and dividend are positive.  */
	jmp	__ia16_ldivmodu
.p_divor_n_divnd:
	/*
	 * Divisor is positive, dividend is negative.  Negate the dividend,
	 * then negate the results.
	 *
	 *	(-17L) / 3L == -(17L / 3L) == -5L
	 *	(-17L) % 3L == -(17L % 3L) == -2L
	 */
	call	.negate_dxax
	call	__ia16_ldivmodu
	call	.negate_cxbx
	jmp	.negate_dxax
.n_divor:
	call	.negate_cxbx
	test	%dx,	%dx
	js	.n_divor_n_divnd
	/*
	 * Divisor is negative, dividend is positive.  Negate the divisor,
	 * then negate only the quotient.
	 *
	 *	17L / (-3L) == -(17L / 3L) == -5L
	 *	17L % (-3L) == 17L % 3L == 2L
	 */
	call	__ia16_ldivmodu
.negate_dxax:
	negw	%ax
	adcw	$0,	%dx
	negw	%dx
	ret
.n_divor_n_divnd:
	/*
	 * Both the divisor and dividend are negative.  Negate both
	 * operands, then negate only the remainder.
	 *
	 *	(-17L) / (-3L) == 17L / 3L == 5L
	 *	(-17L) % (-3L) == -(17L % 3L) == -2L
	 */
	call	.negate_dxax
	call	__ia16_ldivmodu
.negate_cxbx:
	negw	%bx
	adcw	$0,	%cx
	negw	%cx
	ret
#elif defined L_ashlsi3
	.global	__ashlsi3
	.type	__ashlsi3, @function
__ashlsi3:
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
	andw	$0x1f,	%cx
# ifdef __OPTIMIZE_SIZE__
	jcxz	1f
0:
	shlw	%ax
	rclw	%dx
	loop	0b
1:
	ret
# else
	testb	$0x10,	%cl
	jz	0f
	movw	%ax,	%dx
	xorw	%ax,	%ax
	subb	$0x10,	%cl
	shlw	%cl,	%dx
	ret
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
	ret
# endif
#elif defined L_lshrsi3
	.global	__lshrsi3
	.type	__lshrsi3, @function
__lshrsi3:
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
	andw	$0x1f,	%cx
# ifdef __OPTIMIZE_SIZE__
	jcxz	1f
0:
	shrw	%dx
	rcrw	%ax
	loop	0b
1:
	ret
# else
	testb	$0x10,	%cl
	jz	0f
	movw	%dx,	%ax
	xorw	%dx,	%dx
	subb	$0x10,	%cl
	shrw	%cl,	%ax
	ret
0:
	movw	%dx,	%bx
	shrw	%cl,	%dx
	rorw	%cl,	%bx
	xorw	%dx,	%bx
	shrw	%cl,	%ax
	orw	%bx,	%ax
	ret
# endif
#elif defined L_ne_sf
	/* Cheat a bit here: define __nesf2, __ltsf2, and __lesf2 in the
	   same module, since we can collapse all of them to the same jump
	   anyway.  The modules for __ltsf2 and __lesf2 do not need to
	   define anything then.  Ditto for __*df2.

	   It would be better if ELF supports aliases to external symbols
	   (blog.omega-prime.co.uk/2011/07/06/the-sad-state-of-symbol-aliases/)
	   --- then we can get rid of even the jump.  -- tkchia 20190322 */
	.global	__nesf2
	.global	__ltsf2
	.global	__lesf2
__nesf2:
__ltsf2:
__lesf2:
	jmp	__eqsf2
#elif defined L_gt_sf
	.global	__gtsf2
__gtsf2:
	jmp	__gesf2
#elif defined L_ne_df
	.global	__nedf2
	.global	__ltdf2
	.global	__ledf2
__nedf2:
__ltdf2:
__ledf2:
	jmp	__eqdf2
#elif defined L_gt_df
	.global	__gtdf2
__gtdf2:
	jmp	__gedf2
#elif defined L_ia16_far_call_thunk_bx_xx
	.global	__ia16_far_call_thunk.bx.si
	.type	__ia16_far_call_thunk.bx.si, @function
	.global	__ia16_far_call_thunk.bx.di
	.type	__ia16_far_call_thunk.bx.di, @function
__ia16_far_call_thunk.bx.si:
	pushw	%bp			/* set up stack frame */
	movw	%sp,	%bp
	pushw	%bx			/* preserve %bx and %cx */
	pushw	%cx
	movw	%si,	%cx		/* move stack args. size to %cx */
	jmp	0f
__ia16_far_call_thunk.bx.di:
	pushw	%bp
	movw	%sp,	%bp
	pushw	%bx
	pushw	%cx
	movw	%di,	%cx
0:					/* re-push stack arguments */
	jcxz	2f
	leaw	4(%bp),	%bx
	addw	%cx,	%bx
	shrw	%cx
1:
	pushw	%ss:(%bx)
	decw	%bx
	decw	%bx
	loop	1b
2:
	movw	-4(%bp), %cx		/* reload %cx for callee */
	callw	*-2(%bp)		/* call function */
	movw	%bp,	%sp		/* tear down stack frame and return */
	popw	%bp
	lretw
#elif defined L_ia16_far_call_pop_thunk_bx_xx_xx
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
#endif
