;  Peephole optimizations for Intel 16-bit x86.
;  Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
;  Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>
;
;  This file is part of GCC.
;
;  GCC is free software; you can redistribute it and/or modify it under the
;  terms of the GNU General Public License as published by the Free Software
;  Foundation; either version 3 of the License, or (at your option) any
;  later version.
;
;  GCC is distributed in the hope that it will be useful, but WITHOUT ANY
;  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
;  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
;  details.
;
;  You should have received a copy of the GNU General Public License along
;  with this program.  If not, see <http://www.gnu.org/licenses/>.


; Avoid changing the mode or register number of operands because this
; prevents e.g. the rnreg pass from optimizing the code. Likewise,
; avoid adding output operands.

; TODO list
; (This one might happen automatically when inc/dec CC is fixed.)
; cmp $-1/$1, scratch_reg; je/jne -> inc/dec scratch_reg; je/jne

; cmp $80/$8000, QI/HIdest; jb/ja -> (cmp $0, dest;) jns/js

; From unwinde-dw2-fde.o (search_object):
;     c78:       8b 76 ce                mov    -50(%bp),%si
;     c7b:       2b 76 d4                sub    -44(%bp),%si
;     c7e:       d1 fe                   sar    %si
;     c80:       d1 e6                   shl    %si
;     c82:       83 c6 04                add    $0x4,%si

; Contents:
; Move multiple peepholes.
; Misc peepholes.
; And with constants.
; Ior with constants.
; Xor with constants.
; Shift/rotate with constants.

;; Move multiple peepholes.

; Optimize "movb $12, mem; movb $34, mem+1" into "movw $3412, mem".
; Optimize "movb $12, %cl; movb $34, %ch" into "movw $3412, %cx".
; TODO: We want "xorw %dx" from "movb $0, %dl; movb $0, %dh".
(define_peephole2
  [(set (match_operand:QI 0 "nonimmediate_operand")
	(match_operand:QI 2 "const_int_operand"))
   (set (match_operand:QI 1 "nonimmediate_operand")
	(match_operand:QI 3 "const_int_operand"))]
  "(MEM_P (operands[0])
      && MEM_P (operands[1])
      && ia16_move_multiple_mem_p (QImode, operands[0], operands[1]))
    || (REG_P (operands[0])
      && REG_P (operands[1])
      && ia16_move_multiple_reg_p (QImode, operands[0], operands[1]))"
  [(parallel
  [(set (match_dup 0) (match_dup 2))
   (set (match_dup 1) (match_dup 3))]
  )]
)

; Optimize "movb $12, mem+1; movb $34, mem" into "movw $1234, mem".
; Optimize "movb $12, %ch; movb $34, %cl" into "movw $1234, %cx".
; TODO: We want "xorw %dx" from "movb $0, %dh; movb $0, %dl".
(define_peephole2
  [(set (match_operand:QI 0 "nonimmediate_operand")
	(match_operand:QI 2 "const_int_operand"))
   (set (match_operand:QI 1 "nonimmediate_operand")
	(match_operand:QI 3 "const_int_operand"))]
  "(MEM_P (operands[0])
      && MEM_P (operands[1])
      && ia16_move_multiple_mem_p (QImode, operands[1], operands[0]))
    || (REG_P (operands[0])
      && REG_P (operands[1])
      && ia16_move_multiple_reg_p (QImode, operands[1], operands[0]))"
  [(parallel
  [(set (match_dup 1) (match_dup 3))
   (set (match_dup 0) (match_dup 2))]
  )]
)

; Optimize "movb %al, mem; movb %ah, mem+1" into "movw %ax, mem".
(define_peephole2
  [(set (match_operand:QI 0 "memory_operand")
	(match_operand:QI 2 "register_operand"))
   (set (match_operand:QI 1 "memory_operand")
	(match_operand:QI 3 "register_operand"))]
  "ia16_move_multiple_mem_p (QImode, operands[0], operands[1])
   && ia16_move_multiple_reg_p (QImode, operands[2], operands[3])"
  [(parallel
  [(set (match_dup 0) (match_dup 2))
   (set (match_dup 1) (match_dup 3))]
  )]
)

; Optimize "movb %ah, mem+1; movb %al, mem" into "movw %ax, mem".
; For example foo() in testsuite/gcc.c-torture/execute/va-arg-22.c.
(define_peephole2
  [(set (match_operand:QI 0 "memory_operand")
	(match_operand:QI 2 "register_operand"))
   (set (match_operand:QI 1 "memory_operand")
	(match_operand:QI 3 "register_operand"))]
  "ia16_move_multiple_mem_p (QImode, operands[1], operands[0])
   && ia16_move_multiple_reg_p (QImode, operands[3], operands[2])"
  [(parallel
  [(set (match_dup 1) (match_dup 3))
   (set (match_dup 0) (match_dup 2))]
  )]
)

; Optimize "movb %ah, mem; movb %al, mem+1" into "xchgb %al, %ah; movw %ax, mem"
; if %al and %ah die here.
(define_peephole2
  [(set (match_operand:QI 0 "memory_operand")
	(match_operand:QI 2 "register_operand"))
   (set (match_operand:QI 1 "memory_operand")
	(match_operand:QI 3 "register_operand"))]
  "ia16_move_multiple_mem_p (QImode, operands[0], operands[1])
   && ia16_move_multiple_reg_p (QImode, operands[3], operands[2])
   && peep2_reg_dead_p (1, operands[2])
   && peep2_reg_dead_p (2, operands[3])"
  [(parallel
  [(set (match_dup 2) (match_dup 3))
   (set (match_dup 3) (match_dup 2))
  ])
   (parallel
  [(set (match_dup 0) (match_dup 3))
   (set (match_dup 1) (match_dup 2))]
  )]
)

; Optimize "movb mem, %cl; movb mem+1, %ch" into "movw mem, %cx".
(define_peephole2
  [(set (match_operand:QI 0 "register_operand")
	(match_operand:QI 2 "memory_operand"))
   (set (match_operand:QI 1 "register_operand")
	(match_operand:QI 3 "memory_operand"))]
  "ia16_move_multiple_mem_p (QImode, operands[2], operands[3])
   && ia16_move_multiple_reg_p (QImode, operands[0], operands[1])"
  [(parallel
  [(set (match_dup 0) (match_dup 2))
   (set (match_dup 1) (match_dup 3))]
  )]
)

; Optimize "movb mem+1, %ch; movb mem, %cl" into "movw mem, %cx".
(define_peephole2
  [(set (match_operand:QI 0 "register_operand")
	(match_operand:QI 2 "memory_operand"))
   (set (match_operand:QI 1 "register_operand")
	(match_operand:QI 3 "memory_operand"))]
  "ia16_move_multiple_mem_p (QImode, operands[3], operands[2])
   && ia16_move_multiple_reg_p (QImode, operands[1], operands[0])"
  [(parallel
  [(set (match_dup 1) (match_dup 3))
   (set (match_dup 0) (match_dup 2))]
  )]
)

(define_insn "*load_multipleqi"
  [(set (match_operand:QI 0 "register_operand" "=l")
	(match_operand:QI 1 "nonregister_operand" "mn"))
   (set (match_operand:QI 2 "register_operand" "=u")
	(match_operand:QI 3 "nonregister_operand" "mn"))]
  "reload_completed
   && ((CONST_INT_P (operands[1]) && CONST_INT_P (operands[3]))
      || (MEM_P (operands[1]) && MEM_P (operands[3])
	 && ia16_move_multiple_mem_p (QImode, operands[1], operands[3])))
   && ia16_move_multiple_reg_p (QImode, operands[0], operands[2])"
{
  operands[4] = simplify_gen_subreg (HImode, operands[0], QImode, 0);
  if (CONST_INT_P (operands[1]) && CONST_INT_P (operands[3]))
    operands[5] = gen_rtx_CONST_INT (HImode, INTVAL (operands[1])
				     + INTVAL (operands[3]) * 256);
  else
    operands[5] = widen_memory_access (operands[1], HImode, 0);
  return "movw\t%5,\t%4";
})

(define_insn "*store_multipleqi"
  [(set (match_operand:QI 0 "memory_operand" "=m")
	(match_operand:QI 1 "nonmemory_operand" "ln"))
   (set (match_operand:QI 2 "memory_operand" "=m")
	(match_operand:QI 3 "nonmemory_operand" "un"))]
  "reload_completed
   && ((CONST_INT_P (operands[1]) && CONST_INT_P (operands[3]))
      || (REG_P (operands[1]) && REG_P (operands[3])
	 && ia16_move_multiple_reg_p (QImode, operands[1], operands[3])))
   && ia16_move_multiple_mem_p (QImode, operands[0], operands[2])"
{
  if (CONST_INT_P (operands[1]) && CONST_INT_P (operands[3]))
    operands[4] = gen_rtx_CONST_INT (HImode, INTVAL (operands[1])
				     + INTVAL (operands[3]) * 256);
  else
    operands[4] = simplify_gen_subreg (HImode, operands[1], QImode, 0);
  operands[5] = widen_memory_access (operands[0], HImode, 0);
  return "movw\t%4,\t%5";
})

; Optimize "movw mem, reg16; movw mem+2, %es" into "lesw mem, reg16".
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "memory_operand"))
   (set (match_operand:HI 2 "segment_register_operand")
	(match_operand:HI 3 "memory_operand"))]
  "TEST_HARD_REG_BIT (reg_class_contents[GENERAL_REGS], REGNO (operands[0]))
   && ia16_move_multiple_mem_p (HImode, operands[1], operands[3])"
  [(parallel
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))]
  )]
)

; Optimize "movw mem+2, %es; movw mem, reg16" into "lesw mem, reg16".
(define_peephole2
  [(set (match_operand:HI 2 "segment_register_operand")
	(match_operand:HI 3 "memory_operand"))
   (set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "memory_operand"))]
  "TEST_HARD_REG_BIT (reg_class_contents[GENERAL_REGS], REGNO (operands[0]))
   && ia16_move_multiple_mem_p (HImode, operands[1], operands[3])"
  [(parallel
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 2) (match_dup 3))]
  )]
)

; Try this optimization (from libgcc/udivmodsi4.o):
;	movw	42(%bp),%ax	   lesw	42(%bp),%ax
;	movw	%ax,	12(%bp)	-> movw	%ax,	12(%bp)
;	movw	44(%bp),%es
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "memory_operand"))
   (set (match_operand:MO 2 "memory_operand")
	(match_operand:MO 3 "nonmemory_operand"))
   (set (match_operand:HI 4 "segment_register_operand")
	(match_operand:HI 5 "memory_operand"))]
  "TEST_HARD_REG_BIT (reg_class_contents[GENERAL_REGS], REGNO (operands[0]))
   && ia16_move_multiple_mem_p (HImode, operands[1], operands[5])
   && !reg_overlap_mentioned_p (operands[4], operands[3])
   && ia16_non_overlapping_mem_p (operands[2], operands[5])"
  [(parallel
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 4) (match_dup 5))
  ])
   (set (match_dup 2) (match_dup 3))]
)

; Seen in many places (e.g. _udivdi3.o).
; movw	mem1,	%tmp1		lesw	mem1,	%tmp1
; movw	%tmp1,	mem2	->	movw	%tmp1,	mem2
; movw	mem1+2,	%tmp2
; movw	%tmp2,	mem3		movw	%es,	mem3
;REGNO (operands[0]) != ES_REG
;   && (df_regs_ever_live_p (ES_REG) || call_used_regs[ES_REG])
;   && peep2_regno_dead_p (0, ES_REG)
;   && 
(define_peephole2
  [(match_scratch:HI 6 "Q")
   (set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "memory_operand"))
   (set (match_operand:HI 2 "memory_operand")
	(match_dup 0))
   (set (match_operand:HI 3 "register_operand")
	(match_operand:HI 4 "memory_operand"))
   (match_dup 6)
   (set (match_operand:HI 5 "memory_operand")
	(match_dup 3))]
  "peep2_reg_dead_p (4, operands[3])
   && ia16_move_multiple_mem_p (HImode, operands[1], operands[4])"
  [(parallel
  [(set (match_dup 0) (match_dup 1))
   (set (match_dup 6) (match_dup 4))
  ])
   (set (match_dup 2) (match_dup 0))
   (set (match_dup 5) (match_dup 6))]
)

; TODO: (libm _kernel_standard)
; movw	mem1,	%es		lesw	mem1,	%tmp
; movw	%es,	mem2	->	movw	%tmp,	mem2
; movw	mem1+2,	%es
; movw	%es,	mem3		movw	%es,	mem3

(define_insn "*load_multiplehi"
  [(set (match_operand:HI 0 "register_operand" "=r")
	(match_operand:HI 1 "memory_operand" "m"))
   (set (match_operand:HI 2 "segment_register_operand" "=Q")
	(match_operand:HI 3 "memory_operand" "m"))]
  "reload_completed
   && ia16_move_multiple_mem_p (HImode, operands[1], operands[3])"
  "l%R2w\t%1,\t%0"
)

;; Misc peepholes.
; Optimize "mov $0,reg" into "xor reg,reg" if we can clobber CC_REG.
(define_peephole2
  [(set (match_operand:MO 0 "register_operand" "")
	(const_int 0))]
  "TEST_HARD_REG_BIT (reg_class_contents[GENERAL_REGS], REGNO (operands[0]))
   && peep2_regno_dead_p (1, CC_REG)"
  [(parallel
  [(set (match_dup 0) (const_int 0))
   (clobber (reg:CC CC_REG))]
   )]
  ""
)

; Optimize EQ/NE comparison of a register against zero followed by a branch
; from CCZ_Cmode to CCZ_mode, saving one byte.
(define_peephole2
  [(set (reg:CCZ_C CC_REG)
	(compare:CCZ_C (match_operand:MO 0 "register_operand") (const_int 0)))
   (set (pc) (if_then_else (any_cond_z (reg:CCZ_C CC_REG) (const_int 0))
			   (label_ref (match_operand 1))
			   (pc)))]
  "peep2_regno_dead_p (2, CC_REG)"
  [(set (reg:CCZ CC_REG) (compare:CCZ (match_dup 0) (const_int 0)))
   (set (pc) (if_then_else (any_cond_z (reg:CCZ CC_REG) (const_int 0))
			   (label_ref (match_dup 1))
			   (pc)))]
)

; Similarly for LT/GE: Change CCS_NCmode into CCSmode.
(define_peephole2
  [(set (reg:CCS_NC CC_REG)
	(compare:CCS_NC (match_operand:MO 0 "register_operand") (const_int 0)))
   (set (pc) (if_then_else (any_cond_so (reg:CCS_NC CC_REG) (const_int 0))
			   (label_ref (match_operand 1))
			   (pc)))]
  "peep2_regno_dead_p (2, CC_REG)"
  [(set (reg:CCS CC_REG) (compare:CCS (match_dup 0) (const_int 0)))
   (set (pc) (if_then_else (any_cond_so (reg:CCS CC_REG) (const_int 0))
			   (label_ref (match_dup 1))
			   (pc)))]
)

; Convert "add $-2, %sp" into "push %ds" and "add $-4, %sp" into
; "push %ds; push %ds" when size matters.  Use pusha on i80186+.
(define_insn "*subhi3_sp_const2_4_16_18_32"
  [(set (reg:HI SP_REG)
	(minus:HI (reg:HI SP_REG)
		  (match_operand:HI 0 "const_int_operand" "n")))
   (clobber (reg:CC CC_REG))]
  "optimize_size && ((INTVAL (operands[0]) == 2 || INTVAL (operands[0]) == 4)
   || (TARGET_PUSHA && (INTVAL (operands[0]) == 16 || INTVAL (operands[0]) == 18
       || INTVAL (operands[0]) == 32)))"
{
  switch (INTVAL (operands[0]))
    {
      case 32:
	return ("pusha\;pusha");
      case 18:
	return ("pushw\t%%ds\;pusha");
      case 16:
	return ("pusha");
      case  4:
	return ("pushw\t%%ds\;pushw\t%%ds");
      case  2:
	return ("pushw\t%%ds");
      default:
	gcc_unreachable ();
    }
})

; Convert "add $2, %sp" into "pop %scratch" when optimizing for size.
; Convert "add $4, %sp" into "pop %scratch; pop %scratch" when optimizing
; for size.
(define_peephole2
  [(match_scratch:HI 1 "R")
   (parallel [
   (set (reg:HI SP_REG) (plus:HI (reg:HI SP_REG)
				 (match_operand:HI 0 "const_int_operand")))
   (clobber (reg:CC CC_REG))]
   )]
   "optimize_size && ((INTVAL (operands[0]) == 2 || INTVAL (operands[0]) == 4))"
  [(parallel
  [(set (reg:HI SP_REG)	(plus:HI (reg:HI SP_REG) (match_dup 0)))
   (clobber (match_dup 1))]
   )]
   ""
)

(define_insn "*addhi3_sp_const2_4"
  [(set (reg:HI SP_REG)
	(plus:HI (reg:HI SP_REG)
		  (match_operand:HI 0 "const_int_operand" "n")))
   (clobber (match_scratch:HI 1 "=T"))]
  "optimize_size && reload_completed
   && (INTVAL (operands[0]) == 2 || INTVAL (operands[0]) == 4)"
{
  switch (INTVAL (operands[0]))
    {
      case  4:
	return ("popw\t%1\;popw\t%1");
      case  2:
	return ("popw\t%1");
      default:
	gcc_unreachable ();
    }
})


; 'sub $-128, dest' is one byte shorter than 'add $128, dest'.
; TODO Non-clobber versions.
(define_peephole2
	[(parallel
	[(set (match_operand:MO 0 "nonimmediate_operand")
	      (plus:MO (match_dup 0) (const_int 128)))
	 (clobber (reg:CC CC_REG))]
	)]
	""
	[(parallel
	[(set (match_dup 0) (minus:MO (match_dup 0) (const_int -128)))
	 (clobber (reg:CC CC_REG))]
	)]
	""
)

; 'add $-128, dest' is one byte shorter than 'sub $128, dest'.
; TODO Non-clobber versions.
(define_peephole2
	[(parallel
	[(set (match_operand:MO 0 "nonimmediate_operand")
	      (minus:MO (match_dup 0) (const_int 128)))
	 (clobber (reg:CC CC_REG))]
	)]
	""
	[(parallel
	[(set (match_dup 0) (plus:MO (match_dup 0) (const_int -128)))
	 (clobber (reg:CC CC_REG))]
	)]
	""
)

; "and" can sign-extend an 8-bit immediate. "test" can't. Optimize
; 0:	f7 c3 55 00	test	$0x55,%bx
; 4:	89 c3		mov	%ax,%bx
; 6:	75 xx		jne	xx <fubar>
; into
; 0:	83 e3 55	and	$0x55,%bx
; 3:	89 c3		mov	%ax,%bx
; 5:	75 xx		jne	xx <fubar>
; when bx is a scratch register.
; FIXME Documentation doesn't say you can't use constraints except in
; match_scratch.
(define_peephole2
  [(set (reg:SET_CC CC_REG)
	(compare:SET_CC (and:MO (match_operand:MO 0 "register_operand")
				(match_operand:MO 1 "general_operand"))
			(const_int 0)))]
  "peep2_reg_dead_p (1, operands[0])"
  [(parallel
  [(set (reg:SET_CC CC_REG)
	(compare:SET_CC (and:MO (match_dup 0) (match_dup 1))
			(const_int 0)))
   (clobber (match_dup 0))]
  )]
  ""
)

; Try to rewrite
;	movw	$648,	%bp
;	addw	%sp,	%bp
;	movw	%sp,	%si
;	movw	126(%si), %si
;	leaw	-320(%bp,%si), %di
; into
;	movw	%sp,	%si
;	movw	126(%si), %bp
;	leaw	328(%bp,%si),%di
; if %bp and %si die here. Reload generates this sequence.
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "const_int_operand"))
   (parallel [
   (set (match_dup 0) (plus:HI  (match_dup 0)
				(match_operand:HI 2 "register_operand")))
   (clobber (reg CC_REG))
   ])
   (set (match_operand:HI 5 "register_operand")	(match_dup 2))
   (set (match_dup 5) (match_operand:HI 6 "general_operand"))
   (set (match_operand:HI 3 "register_operand")
	(plus:HI (plus:HI (match_dup 0) (match_dup 5))
		 (match_operand:HI 4 "immediate_operand")))]
  "!reg_mentioned_p (operands[0], operands[6])
   && (rtx_equal_p (operands[0], operands[3])
       || peep2_reg_dead_p (5, operands[0]))
   && (rtx_equal_p (operands[5], operands[3])
       || peep2_reg_dead_p (5, operands[5]))"
  [(set (match_dup 5) (match_dup 2))
   (set (match_dup 0) (match_dup 6))
   (set (match_dup 3) (plus:HI  (plus:HI (match_dup 0) (match_dup 5))
				(match_dup 7)))]
{
  operands[7] = plus_constant (operands[4], INTVAL (operands[1]));
})

; Try to rewrite
;	movw	$1100,	%bp
;	addw	%sp,	%bp
;	movw	%cx,	%di	; optional
;	leaw	-400(%bp,%di), %cx
; into
;	movw	%sp,	%bp
;	movw 	%cx,	%di	; optional
;	leaw	700(%bp,%di), %cx
; if %bp dies here. This sequence is generated during reload.
; TODO: Try to change "const_int_operand" into "immediate_operand".
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "const_int_operand"))
   (parallel [
   (set (match_dup 0) (plus:HI  (match_dup 0)
				(match_operand:HI 2 "register_operand")))
   (clobber (reg CC_REG))
   ])
   (set (match_operand:HI 5 "register_operand")
	(match_operand:HI 6 "general_operand"))
   (set (match_operand:HI 3 "register_operand")
	(plus:HI (plus:HI (match_dup 0) (match_dup 5))
		 (match_operand:HI 4 "immediate_operand")))]
  "peep2_reg_dead_p (4, operands[0])"
  [(set (match_dup 0) (match_dup 2))
   (set (match_dup 5) (match_dup 6))
   (set (match_dup 3)
	(plus:HI (plus:HI (match_dup 0) (match_dup 5))
		 (match_dup 7)))]
{
  operands[7] = plus_constant (operands[4], INTVAL (operands[1]));
})

(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
	(match_operand:HI 1 "const_int_operand"))
   (parallel [
   (set (match_dup 0) (plus:HI  (match_dup 0)
				(match_operand:HI 2 "register_operand")))
   (clobber (reg CC_REG))
   ])
   (set (match_operand:HI 3 "register_operand")
	(plus:HI (plus:HI (match_dup 0)
			  (match_operand:HI 5 "register_operand"))
		 (match_operand:HI 4 "immediate_operand")))]
  "peep2_reg_dead_p (3, operands[0])"
  [(set (match_dup 0) (match_dup 2))
   (set (match_dup 3)
	(plus:HI (plus:HI (match_dup 0) (match_dup 5))
		 (match_dup 6)))]
{
  operands[6] = plus_constant (operands[4], INTVAL (operands[1]));
})

;; And with constants.

; Recursive peepholes would help - xor reg,reg vs. mov $0,mem.

; TODO: In unwind-dw2-fde.o (__register_frame_info_table_bases), we get:
;      f7:       c7 47 08 00 00          movw   $0x0,8(%bx)
;      fc:       80 4f 08 02             orb    $0x2,8(%bx)
;     100:       81 4f 08 f8 07          orw    $0x7f8,8(%bx)
; This should be just "movw $0x7fa, 8(%bx)".

; TODO: Despite the pattern below, I still see sequences like this in
; unwinde-dw2-fde.o (search_object):
;    1145:       80 67 09 07             andb   $0x7,9(%bx)
;    1149:       81 67 0a 00 f8          andw   $0xf800,10(%bx)
;    114e:       80 67 0b 07             andb   $0x7,11(%bx)

; Optimize "and $-x, dest; and $x-1, dest" into "mov $0, dest".
; TODO: This should be machine independent.
(define_peephole2
  [(parallel
  [(set (match_operand:MO 0 "nonimmediate_operand")
	(and:MO (match_dup 0)
		(match_operand:MO 1 "immediate_operand")))
   (clobber (reg:CC CC_REG))
   ])
   (parallel [
   (set (match_dup 0) (and:MO (match_dup 0)
			      (match_operand:MO 2 "immediate_operand")))
   (clobber (reg:CC CC_REG))]
   )]
   "INTVAL (operands[1]) + INTVAL (operands[2]) == -1"
   [(set (match_dup 0) (const_int 0))]
   ""
)

(define_insn "*andhi3_const255_noclobber"
	[(set (match_operand:HI 0 "nonimmediate_operand" "=qm")
	      (and:HI (match_operand:HI 1 "nonimmediate_operand" "0")
	              (const_int 255)))]
	""
{
	return "movb\t$0,\t%H0";
})

; Optimize "movw mem, reg; andw $x, reg" into "movb mem, reg; andw $x, reg"
; for x & 0xff00 == 0 to speed up execution on 8-bit CPUs.
; This can't really be done before reload because pseudos may turn out to be
; memory and thus miss this optimization.
(define_peephole2
  [(set (match_operand:HI 0 "register_operand")
  	(match_operand:HI 1 "memory_operand"))
   (parallel [
   (set (match_dup 0) (and:HI (match_dup 0)
  			      (match_operand:HI 2 "const_int_operand")))
   (clobber (reg:CC CC_REG))]
   )]
  "(INTVAL (operands[2]) & 0xff00) == 0
   && TEST_HARD_REG_BIT (reg_class_contents[LO_QI_REGS], REGNO (operands[0]))"
  [(clobber (match_dup 3))
   (set (match_dup 4)
  	(match_dup 5))
   (parallel [
   (set (match_dup 0) (and:HI (match_dup 0)
  			      (match_operand:HI 2 "const_int_operand")))
   (clobber (reg:CC CC_REG))]
   )]
{
  operands[3] = simplify_gen_subreg (QImode, operands[0], HImode, 1);
  operands[4] = simplify_gen_subreg (QImode, operands[0], HImode, 0);
  operands[5] = simplify_gen_subreg (QImode, operands[1], HImode, 0);
})
