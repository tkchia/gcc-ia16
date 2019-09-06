	.arch i8086,jumps
	.code16
	.att_syntax prefix

	.text

#if defined L_lshrsi3
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
