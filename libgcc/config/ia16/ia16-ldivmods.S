#include "ia16-preamble.h"

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
