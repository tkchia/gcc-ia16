/* GNU Compiler Collection definitions of target machine Intel 16-bit x86.
   Copyright (C) 2005-2017 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>
   Changes by Andrew Jenner <andrew@codesourcery.com>
   Very preliminary IA-16 far pointer support and other changes by TK Chia

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

/* Keep this in sync with processor_target_table in ia16.c.  */
#define TARGET_SHIFT_IMM	(ia16_features & 1)
#define TARGET_PUSH_IMM		(ia16_features & 1)
#define TARGET_IMUL_IMM		(ia16_features & 1)
#define TARGET_PUSHA		(ia16_features & 1)
#define TARGET_ENTER_LEAVE	(ia16_features & 1)
#define TARGET_SHIFT_MASKED	(ia16_features & 2)
#define TARGET_AAD_IMM		(ia16_features & 4)
#define TARGET_FSTSW_AX		(ia16_features & 8)
#define TARGET_TUNE_8BIT	(ia16_features & 16)

#define TARGET_ALLOCABLE_DS_REG	(! fixed_regs[DS_REG])
#define TARGET_CMODEL_IS_TINY	(target_cmodel == CMODEL_TINY)
#define TARGET_CMODEL_IS_FAR_TEXT (target_cmodel == CMODEL_MEDIUM || \
				   target_cmodel == CMODEL_LARGE || \
				   target_cmodel == CMODEL_HUGE)

/* Run-time Target Specification */
#define TARGET_CPU_CPP_BUILTINS() ia16_cpu_cpp_builtins ()

/* Storage Layout
 *
 * A unit is 8 bits, a machine word is 16 bits (two units), all little endian.
 */

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0
#define UNITS_PER_WORD 2

#define PARM_BOUNDARY		BITS_PER_WORD
#define STACK_BOUNDARY		BITS_PER_WORD
#define FUNCTION_BOUNDARY	BITS_PER_UNIT
#define BIGGEST_ALIGNMENT	BITS_PER_WORD
#define STRUCTURE_SIZE_BOUNDARY	BITS_PER_UNIT
#define STRICT_ALIGNMENT	0
#define PCC_BITFIELD_TYPE_MATTERS 1

/* Layout of Source Language Data Types.  */
#define         INT_TYPE_SIZE 16
#define       SHORT_TYPE_SIZE 16
#define        LONG_TYPE_SIZE 32
#define   LONG_LONG_TYPE_SIZE 64
#define       FLOAT_TYPE_SIZE 32
#define      DOUBLE_TYPE_SIZE 64
#define LONG_DOUBLE_TYPE_SIZE 64

#define DEFAULT_SIGNED_CHAR		1
#define SIZE_TYPE			"unsigned int"
#define PTRDIFF_TYPE			"int"
#define WCHAR_TYPE_SIZE			16

#define TARGET_VTABLE_USES_DESCRIPTORS 0

/* Register Usage
 *
 * These are the registers of the 8086 family.
 * The argument pointer register (ap/argp) is a fixed, virtual hard register.
 *
 * The order matters.  dh:dl:ah:al should be consequtive because some
 * instructions treat them as a 32-bit register.  Cl should be at one of the
 * ends or it will be impossible to fit both a shift count and a 64-bit
 * value at the same time.  Life is a lot easier if xl:xh pairs are kept
 * together.  We don't want to tie up both index (si and di) or both base w/
 * index (bx and bp) registers with a value spanning multiple hard
 * registers.
 *
 * (I include SS_REG and CS_REG in the register file to make it easier to
 * express %ds <- %ss assignments and near -> far pointer casts.  They play
 * no part in register allocation.  -- tkchia)
 *
 * TODO: For optimization purposes, we'd want a 64-bit value to be held in
 * registers such that the upper and lower bytes are in an 8-bit register.
 *
 * Name:  cl ch al ah dl dh bl bh si di bp es ds sp cc ss cs ap
 * Size:   8  8  8  8  8  8  8  8 16 16 16 16 16 16 cc 16 16 16
 * Index:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17
 *
 * Basic Characteristics of Registers
 */
#define FIXED_REGISTERS \
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 }
#define CALL_USED_REGISTERS \
         { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 }

/* Order of Allocation of Registers.  */
/* The goal is to create opportunities for using load/store multiple
 * instructions as well as good register allocation in general.  */
#define REG_ALLOC_ORDER \
	{ 2, 3, 4, 5, 0, 1,  6, 7, 10,  9, 8, 11, 12, 13, 14, 15, 16, 17 }
/*	 al ah dl dh cl ch  bl bh  bp  di si  es  ds  sp  cc  ss  cs  ap */

/* How Values Fit in Registers.  */
/* FIXME: Not documented: CCmode is 32 bits.  */
/* Must not return 0 or subreg_get_info() may divide by zero.  */
/* FIXME: Handling of XFmode needs to use GET_MODE_PRECISION(). */
#define HARD_REGNO_NREGS(REGNO, MODE) \
  (MAX (ia16_hard_regno_nregs[GET_MODE_SIZE(MODE)][REGNO], 1))

/* There are more cases than those caught here, but HARD_REGNO_MODE_OK()
   forbids them. Catch multireg values that straddle the boundary between
   8-bit and 16-bit registers. */
#define HARD_REGNO_NREGS_HAS_PADDING(REGNO, MODE) \
	((REGNO) < FIRST_NOQI_REG && \
	 (REGNO) + GET_MODE_SIZE(MODE) > FIRST_NOQI_REG)

#define HARD_REGNO_NREGS_WITH_PADDING(REGNO, MODE) \
	(GET_MODE_SIZE(MODE))

#define REGMODE_NATURAL_SIZE(MODE)	\
	(GET_MODE_SIZE(MODE) == 1 || GET_MODE_CLASS(MODE) == MODE_CC ? \
	 1 : UNITS_PER_WORD)

/* Complex modes must not cross the boundary between 8-bit and 16-bit
   registers because subreg_get_info() will fail in that case.  */
#define HARD_REGNO_MODE_OK(REGNO, MODE) \
  (GET_MODE_CLASS(MODE) == MODE_CC ? (REGNO) == CC_REG :		\
   (REGNO) == CC_REG ? GET_MODE_CLASS(MODE) == MODE_CC :		\
   GET_MODE_SIZE(MODE) > 16 ? 0 :					\
   COMPLEX_MODE_P(MODE) &&						\
     HARD_REGNO_NREGS_HAS_PADDING((REGNO), (MODE)) ? 0 :		\
   ia16_hard_regno_nregs[GET_MODE_SIZE(MODE)][REGNO] &&			\
     (! TARGET_PROTECTED_MODE || (MODE) == PHImode			\
      || ((REGNO) != DS_REG && (REGNO) != ES_REG)))

/* For some reason, allowing registers other than %ds to be renamed to %ds
   during the rnreg pass (with e.g. -funroll-loops) leads to incorrect code
   around %ds <- %ss operations, so disable that.

   And, rnreg has this weird idea to rename the segment register in a
   segment override to a non-segment register (argh!).  Disable that too.  */
#define HARD_REGNO_RENAME_OK(FROM, TO) \
	(((FROM) != DS_REG && (FROM) != ES_REG && (TO) != DS_REG) \
	 || ((FROM) == DS_REG && (TO) == ES_REG))

/* When this returns 0, rtx_cost() in rtlanal.c will pessimize the RTX cost
 * estimate, and this particular case cannot be overridden by
 * ia16_rtx_costs().  And if you get it wrong, the register allocator will
 * be stupid about libgcc2.c's _popcountsi2().  The cases we have to reject
 * here are:
 * 1) Access of a 16-bit value in MODE1 as an 8-bit value MODE2.
 *    This will fail for registers in class HI_REGS.
 * 2) Access of an 8-bit value in MODE1 as an 16-bit value in MODE2.
 *    This will fail for registers in class UP_QI_REGS.
 * Used in: rtlanal.c, combine.c, regclass.c and local-alloc.c.
 */
#define MODES_TIEABLE_P(MODE1, MODE2)	\
	(GET_MODE_SIZE(MODE2) > 1 && GET_MODE_SIZE(MODE1) > 1)

/* Handling Leaf Functions
 *
 * Registers That Form a Stack
 * (Hardware floating point is not yet supported.)
 */
/* Register Classes
 *
 * The register classes, along with contraint letter and bit mask.
 * Keep this in sync with REG*_OK_FOR*_P() below.
 * Keep this in sync with ia16_regno_class[] in ia16.c.
 */
enum reg_class {	/*	 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0 */
  NO_REGS,	     /*   000000  4  2  1  4  2  1  4  2  1  4  2  1  4  2  1  4  2  1 */
  AL_REGS,	   /* Ral 000004  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . al  .  . */
  AH_REGS,	   /* Rah 000010  .  .  .  .  .  .  .  .  .  .  .  .  .  . ah  .  .  . */
  AX_REGS,	     /* a 000014  .  .  .  .  .  .  .  .  .  .  .  .  .  . -- ax  .  . */
  DL_REGS,	   /* Rdl 000020  .  .  .  .  .  .  .  .  .  .  .  .  . dl  .  .  .  . */ 
  DH_REGS,	   /* Rdh 000040  .  .  .  .  .  .  .  .  .  .  .  . dh  .  .  .  .  . */
  DX_REGS,	     /* d 000060  .  .  .  .  .  .  .  .  .  .  .  . -- dx  .  .  .  . */
  DXAX_REGS,	     /* A 000074  .  .  .  .  .  .  .  .  .  .  .  . -- dx -- ax  .  . */
  BX_REGS,	     /* b 000300  .  .  .  .  .  .  .  .  .  . -- bx  .  .  .  .  .  . */
  BXDX_REGS,	     /* j 000360  .  .  .  .  .  .  .  .  .  . -- bx -- dx  .  .  .  . */
  CL_REGS,	   /* Rcl 000001  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . cl */
  CX_REGS,	     /* c 000003  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . -- cx */
  LO_QI_REGS,	     /* l 000125  .  .  .  .  .  .  .  .  .  .  . bl  . dl  . al  . cl */
  UP_QI_REGS,	     /* u 000252  .  .  .  .  .  .  .  .  .  . bh  . dh  . ah  . ch  . */
  QI_REGS,	     /* q 000377  .  .  .  .  .  .  .  .  .  . bh bl dh dl ah al ch cl */
  SI_REGS,	     /* S 000400  .  .  .  .  .  .  .  .  . si  .  .  .  .  .  .  .  . */
  QISI_REGS,	     /* k 000777  .  .  .  .  .  .  .  .  . si -- bx -- dx -- ax -- cx */
  DI_REGS,	     /* D 001000  .  .  .  .  .  .  .  . di  .  .  .  .  .  .  .  .  . */
  INDEX_REGS,	     /* x 001400  .  .  .  .  .  .  .  . di si  .  .  .  .  .  .  .  . */
  BP_REGS,	   /* Rbp 002000  .  .  .  .  .  .  . bp  .  .  .  .  .  .  .  .  .  . */
  BASE_W_INDEX_REGS, /* w 002300  .  .  .  .  .  .  . bp  .  . -- bx  .  .  .  .  .  . */
  BASE_REGS,	     /* B 003700  .  .  .  .  .  .  . bp di si -- bx  .  .  .  .  .  . */
  ES_REGS,	     /* e 004000  .  .  .  .  .  . es  .  .  .  .  .  .  .  .  .  .  . */
  DS_REGS,	   /* Rds 010000  .  .  .  .  . ds  .  .  .  .  .  .  .  .  .  .  .  . */
  SEGMENT_REGS,	     /* Q 314000  . cs ss  .  . ds es  .  .  .  .  .  .  .  .  .  .  . */
  HI_REGS,	     /*   337400  . cs ss  . sp ds es bp di si  .  .  .  .  .  .  .  . */
  GENERAL_REGS,	     /* r 023777  .  .  .  . sp  .  . bp di si bh bl dh dl ah al ch cl */
  ES_GENERAL_REGS,   /*   027777  .  .  .  . sp  . es bp di si bh bl dh dl ah al ch cl */
  SEG_GENERAL_REGS,  /* T 337777  . cs ss  . sp ds es bp di si bh bl dh dl ah al ch cl */
  ALL_REGS,	     /*   777777 ap cs ss cc sp ds es bp di si bh bl dh dl ah al ch cl */
  LIM_REG_CLASSES
};
#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define REG_CLASS_NAMES \
{ "NO_REGS", "AL_REGS", "AH_REGS", "AX_REGS", "DL_REGS", "DH_REGS", \
  "DX_REGS", "DXAX_REGS", "BX_REGS", "BXDX_REGS", "CL_REGS", \
  "CX_REGS", "LO_QI_REGS", "UP_QI_REGS", "QI_REGS", \
  "SI_REGS", "QISI_REGS", "DI_REGS", "INDEX_REGS", \
  "BP_REGS", "BASE_W_INDEX_REGS", "BASE_REGS", "ES_REGS", "DS_REGS", \
  "SEGMENT_REGS", "HI_REGS", "GENERAL_REGS", "ES_GENERAL_REGS", \
  "SEG_GENERAL_REGS", "ALL_REGS" }

#define REG_CLASS_CONTENTS {	  /* 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0 */ \
  { 0000000 }, /* NO_REGS,	      4  2  1  4  2  1  4  2  1  4  2  1  4  2  1  4  2  1 */ \
  { 0000004 }, /* AL_REGS,	  Ral .  .  .  .  .  .  .  .  .  .  .  .  .  .  . al  .  . */ \
  { 0000010 }, /* AH_REGS,	  Rah .  .  .  .  .  .  .  .  .  .  .  .  .  . ah  .  .  . */ \
  { 0000014 }, /* AX_REGS,	    a .  .  .  .  .  .  .  .  .  .  .  .  .  . -- ax  .  . */ \
  { 0000020 }, /* DL_REGS,	  Rdl .  .  .  .  .  .  .  .  .  .  .  .  . dl  .  .  .  . */ \
  { 0000040 }, /* DH_REGS,	  Rdh .  .  .  .  .  .  .  .  .  .  .  . dh  .  .  .  .  . */ \
  { 0000060 }, /* DX_REGS,	    d .  .  .  .  .  .  .  .  .  .  .  . -- dx  .  .  .  . */ \
  { 0000074 }, /* DXAX_REGS,	    A .  .  .  .  .  .  .  .  .  .  .  . -- dx -- ax  .  . */ \
  { 0000300 }, /* BX_REGS,	    b .  .  .  .  .  .  .  .  .  . -- bx  .  .  .  .  .  . */ \
  { 0000360 }, /* BXDX_REGS,	    j .  .  .  .  .  .  .  .  .  . -- bx -- dx  .  .  .  . */ \
  { 0000001 }, /* CL_REGS,	  Rcl .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . cl */ \
  { 0000003 }, /* CX_REGS,	    c .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . -- cx */ \
  { 0000125 }, /* LO_QI_REGS,	    l .  .  .  .  .  .  .  .  .  .  . bl  . dl  . al  . cl */ \
  { 0000252 }, /* UP_QI_REGS,	    u .  .  .  .  .  .  .  .  .  . bh  . dh  . ah  . ch  . */ \
  { 0000377 }, /* QI_REGS,	    q .  .  .  .  .  .  .  .  .  . bh bl dh dl ah al ch cl */ \
  { 0000400 }, /* SI_REGS,	    S .  .  .  .  .  .  .  .  . si  .  .  .  .  .  .  .  . */ \
  { 0000777 }, /* QISI_REGS,	    k .  .  .  .  .  .  .  .  . si -- bx -- dx -- ax -- cx */ \
  { 0001000 }, /* DI_REGS,	    D .  .  .  .  .  .  .  . di  .  .  .  .  .  .  .  .  . */ \
  { 0001400 }, /* INDEX_REGS,	    x .  .  .  .  .  .  .  . di si  .  .  .  .  .  .  .  . */ \
  { 0002000 }, /* BP_REGS,	  Rbp .  .  .  .  .  .  . bp  .  .  .  .  .  .  .  .  .  . */ \
  { 0002300 }, /* BASE_W_INDEX_REGS w .  .  .  .  .  .  . bp  .  . -- bx  .  .  .  .  .  . */ \
  { 0003700 }, /* BASE_REGS,	    B .  .  .  .  .  .  . bp di si -- bx  .  .  .  .  .  . */ \
  { 0004000 }, /* ES_REGS,	    e .  .  .  .  .  . es  .  .  .  .  .  .  .  .  .  .  . */ \
  { 0010000 }, /* DS_REGS,	  Rds .  .  .  .  . ds  .  .  .  .  .  .  .  .  .  .  .  . */ \
  { 0314000 }, /* SEGMENT_REGS,	    Q . cs ss  .  . ds es  .  .  .  .  .  .  .  .  .  .  . */ \
  { 0337400 }, /* HI_REGS,	      . cs ss  . sp ds es bp di si  .  .  .  .  .  .  .  . */ \
  { 0023777 }, /* GENERAL_REGS,	    r .  .  .  . sp  .  . bp di si bh bl dh dl ah al ch cl */ \
  { 0027777 }, /* ES_GENERAL_REGS,    .  .  .  . sp  . es bp di si bh bl dh dl ah al ch cl */ \
  { 0337777 }, /* SEG_GENERAL_REGS, T . cs ss  . sp ds es bp di si bh bl dh dl ah al ch cl */ \
  { 0777777 }, /* ALL_REGS,	     ap cs ss cc sp ds es bp di si bh bl dh dl ah al ch cl */ \
}

#define REGNO_REG_CLASS(regno) ia16_regno_class[regno]

/* We use INDEX_REGS for BASE_REG_REGS and BASE_W_INDEX_REGS for INDEX_REGS
 * because this enables GCC to have use two base registers for base+index
 * addressing when not eliminating the frame pointer.  This improves i80286
 * __gcc_bcmp() a lot (or it used to).
 */
#define MODE_BASE_REG_CLASS(mode)	BASE_REGS
#define MODE_BASE_REG_REG_CLASS(mode)	INDEX_REGS
#define INDEX_REG_CLASS			BASE_W_INDEX_REGS

/* We hack gcc/lra-constraints.c to recognize and use a new macro
 * MODE_SEGMENT_REG_CLASS (outer_mode, inner_mode, unspec_op), which should
 * be a set of registers to use for an RTL expression <expr> occurring in a
 * SEGMENT term,
 *	(unspec:<mode> [<expr> ...] <unspec_op>)
 * if this SEGMENT term is part of an address.  For IA-16, the form is
 *	(unspec:HI [<expr>] UNSPEC_SEG_OVERRIDE)
 * and we want to get <expr> loaded into a segment register (well, %es).
 */
#define MODE_SEGMENT_REG_CLASS(outer_mode, inner_mode, unspec_op) \
	((unspec_op) == UNSPEC_SEG_OVERRIDE ? SEGMENT_REGS : NO_REGS)

/* FIXME: Documentation:
 * It is unclear if these definitions should be guarded by REG_OK_STRICT or not.  */
#define REGNO_MODE_OK_FOR_BASE_P(num, mode) \
	ia16_regno_in_class_p ((num), MODE_BASE_REG_CLASS (mode))
#define REGNO_MODE_OK_FOR_REG_BASE_P(num, mode)	\
	ia16_regno_in_class_p ((num), MODE_BASE_REG_REG_CLASS (mode))
#define REGNO_OK_FOR_INDEX_P(num) \
	ia16_regno_in_class_p ((num), INDEX_REG_CLASS)

/* The value returned by CLASS_MAX_NREGS() ends up being used by reload1.c
 * as one of the things considered when sorting reloads.  Reload is very
 * sensitive to this on an arch like IA16.  Reloads potentially requiring
 * many registers are handled first.  So for mixed 8-bit and 16-bit classes,
 * pretend 8-bit registers.  The address register classes should always be
 * considered as requiring four hard regs, although they may in fact require
 * only one, to be sure that they are handled before SImode GENERAL_REGS.
 * DImode GENERAL_REGS is dealt with by HARD_REGNO_MODE_OK().  However, if the
 * class has fewer than four hard regs, regclass.c will decide that the
 * register class is unavailable, in turn ruining local-alloc.c.  So limit this
 * hack to the duration of the reload pass (reload_in_progress).
 * What do you mean "jumping through hoops"?  :-(
 * 
 * Please see reload1.c/reload_reg_class_lower() before changing this.
 * Also see regclass.c/init_reg_sets_1().  This is tricky.
 * Also see regclass.c/record_reg_classes().  This is tricky.
 * Also see reload.c/combine_reloads().  Not so tricky.
 * TODO: A target hook for reload_reg_class_lower().
 * FIXME: CLASS_MAX_NREGS (GENERAL_REGS, XFmode) returns 12.  10 is correct.
 */
#define CLASS_MAX_NREGS(class, mode) \
	((class) == BASE_W_INDEX_REGS && reload_in_progress ? 4 :	\
	 (class) == INDEX_REGS && reload_in_progress ? 4 :		\
	 (class) == BASE_REGS && reload_in_progress ? 4 :		\
	 (class) == BX_REGS && reload_in_progress ? 4 :			\
	 (class) == CX_REGS && reload_in_progress ? 4 :			\
	 (reg_classes_intersect_p (QI_REGS, (class)) ?			\
 	 GET_MODE_SIZE (mode) :						\
	 (GET_MODE_SIZE (mode) + 1U) / 2U) + 0U)

/* HI_REGS cannot change mode to QImode.  We cannot change mode to a
 * larger mode without increasing the number of hard regs used.
 * TODO: This will change when x87 FPUs are supported.  */
#define CANNOT_CHANGE_MODE_CLASS(FROM, TO, CLASS)	\
	(GET_MODE_SIZE(TO) > GET_MODE_SIZE(FROM)	\
	 || ((TO) == QImode && reg_classes_intersect_p (HI_REGS, (CLASS))))

/* Stack Layout and Calling Conventions
 * Basic Stack Layout
 */

#define STACK_GROWS_DOWNWARD 1
#define FRAME_GROWS_DOWNWARD 1
#undef  ARGS_GROW_DOWNWARD
#define STARTING_FRAME_OFFSET 0
#define FIRST_PARM_OFFSET(fundecl) ia16_first_parm_offset (fundecl)

/* Because we have no way of knowing how many registers are saved between the
   return address and the frame pointer, we can't find the return address for
   count > 0.
   TODO: Make this work by pushing the frame pointer before the saved regs
   when not using ENTER.

   For count == 0, set a flag so that ia16_save_reg_p (.) will ultimately
   arrange to allocate a frame pointer, even if the function does not
   otherwise need one.  */
#define RETURN_ADDR_RTX(COUNT, FRAME)				       	      \
	((COUNT) == 0							      \
	 ? crtl->accesses_prior_frames = true,				      \
	   gen_rtx_MEM (Pmode, arg_pointer_rtx)				      \
	 : NULL_RTX)

/* Exception Handling Support */
/* XXX needs work.  */

/* Specifying How Stack Checking is Done */
/* XXX needs work.  */

/* Registers That Address the Stack Frame */
#define STACK_POINTER_REGNUM SP_REG
#define FRAME_POINTER_REGNUM BP_REG
#define   ARG_POINTER_REGNUM ARGP_REG
/* XXX The rest needs work.  */

/* We pick dx to choose the least likely used register.  This needs to be a
   call-used register, because the trampoline will clobber it.  */
#define  STATIC_CHAIN_REGNUM D_REG

/* Eliminating Frame Pointer and Arg Pointer */

/* INITIAL_FRAME_POINTER_OFFSET(depth_var) */
/* FIXME Documentation bug: depth-var is invalid as a macro parameter.  */
/* FIXME Documentation: The offset is in units and is positive if
   STACK_GROWS_UPWARD.  */

#define ELIMINABLE_REGS \
	{ { ARG_POINTER_REGNUM, FRAME_POINTER_REGNUM }, \
	  { STACK_POINTER_REGNUM, FRAME_POINTER_REGNUM } }
#define INITIAL_ELIMINATION_OFFSET(from_reg, to_reg, offset_var) \
	offset_var = ia16_initial_elimination_offset (from_reg, to_reg)

/* Passing Function Arguments on the Stack */
#define PUSH_ARGS		1
#define PUSH_ARGS_REVERSED	1
#define PUSH_ROUNDING(BYTES)	(((BYTES) + 1) & ~1)

/* Passing Arguments in Registers */
/* TODO: Allow arguments to be passed in registers.  */
#define CUMULATIVE_ARGS				int
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS) \
	ia16_init_cumulative_args (&(CUM), (FNTYPE), (LIBNAME), (FNDECL), \
				   (N_NAMED_ARGS))
/* If %ds is an allocatable register, pretend that it "might" be used to
   pass arguments to functions, so that when GCC sees a %ds <- %ss just
   before a subroutine call, it does not try to separate the two.  */
#define FUNCTION_ARG_REGNO_P(regno) \
	((regno) == A_REG \
	 || (regno) == D_REG \
	 || (regno) == C_REG \
	 || (TARGET_ALLOCABLE_DS_REG && (regno) == DS_REG))

/* How Large Values Are Returned */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Caller-Saves Register Allocation */
/* defaults.  */

/* Function Entry and Exit */
/* Thunks support is missing.  */
/* Stack adjustment at function exit isn't needed if we need to tear down the
   stack frame with `leave' or `movw %bp, %sp'.  */
#define EXIT_IGNORE_STACK	(HAVE__leave \
				 || get_frame_size () > 0 \
				 || cfun->calls_alloca \
				 || crtl->outgoing_args_size > 0)

/* Generating Code for Profiling */
/* It isn't there yet.  */
#define FUNCTION_PROFILER(file, labelno) do {} while (0)

/* Permitting tail calls */
/* skipped for now.  */

/* Stack smashing protection */
/* skipped for now.  */

/* Implementing the Varargs Macros */
/* defaults.  */

/* Trampolines for Nested Functions */
#define TRAMPOLINE_SIZE 6

/* Implicit Calls to Library Routines */
/* defaults.  */

/* Addressing Modes */
#define CONSTANT_ADDRESS_P(x)	CONSTANT_P (x)
#define MAX_REGS_PER_ADDRESS	3

/* Condition Code Status */

/* Representation of condition codes using registers */

#define SELECT_CC_MODE(op, x, y)        ia16_select_cc_mode (op, x, y, false)
#define REVERSIBLE_CC_MODE(mode)	1

#define BRANCH_COST(speed_p, predictable_p) \
	ia16_branch_cost (!! (speed_p), !! (predictable_p))
#define SLOW_BYTE_ACCESS	(TARGET_TUNE_8BIT ? 0 : 1)

/* These probably need some tweaking.  Leave the defaults for now.
#define MOVE_RATIO
#define CLEAR_RATIO
*/

/* Loading a function address into a register takes 3 bytes and 4 cycles
 * (i80186) and saves at most 1 byte and 2 cycles (i80186) per call.  So don't
 * waste a precious register on this. TODO: Why does this have no effect?
 */
#define NO_FUNCTION_CSE 1

/* Adjusting the Instruction Scheduler.
   #define TARGET_SCHED_ISSUE_RATE() MAX_DFA_ISSUE_RATE
*/
/* Dividing the Output into Sections (Texts, Data, ...).  */
#define TEXT_SECTION_ASM_OP	"\t.text"
#define DATA_SECTION_ASM_OP	"\t.data"
#define BSS_SECTION_ASM_OP	"\t.bss"
#define CRT_CALL_STATIC_FUNCTION(section_op, function) \
	__IA16___CRT_CALL_STATIC_FUNCTION (section_op, function) /* ia16-c.c */
#define JUMP_TABLES_IN_TEXT_SECTION 0

/* Position Independent Code.  */
/* skipped.  */

/* The Overall Framework of an Assembler File */
#define ASM_APP_ON				"#APP\n"
#define ASM_APP_OFF				"#NO_APP\n"

/* Output of Data.  */

/* Output of Uninitialized Variables.  */

#define ASM_OUTPUT_ALIGNED_BSS(stream, decl, name, size, alignment) \
	asm_output_aligned_bss(stream, decl, name, size, alignment)

/* Output and Generation of Labels.  */

/* Used by target hook TARGET_ASM_GLOBALIZE_LABEL. */
#define GLOBAL_ASM_OP   "\t.global\t"

#undef ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(BUF,PREFIX,NUMBER)  \
  sprintf ((BUF), LOCAL_LABEL_PREFIX "%s%ld", (PREFIX), (long)(NUMBER))

/* Used by target hook TARGET_ASM_GLOBALIZE_LABEL. */
#define GLOBAL_ASM_OP   "\t.global\t"

/* This is obsoleted by TARGET_ASM_EXTERNAL_LIBCALL and wrong in elfos.h.  */
#undef ASM_OUTPUT_EXTERNAL_LIBCALL

/* How Initialization Functions Are Handled.  */
/* skipped for now.*/

/* Output of Assembler Instructions.  */

#define REGISTER_NAMES { "c", "ch", "a", "ah", "d", "dh", "b", "bh", \
                         "si", "di", "bp", "es", "ds", "sp", "cc", \
                         "ss", "cs", "argp" }
#define ADDITIONAL_REGISTER_NAMES \
	{ { "cl", 0 }, {"al", 2 }, { "dl", 4 }, { "bl", 6 } }
#define OVERLAPPING_REGISTER_NAMES \
	{ { "cx", 0, 2 }, {"ax", 2, 2 }, { "dx", 4, 2 }, { "bx", 6, 2 } }
#define PRINT_OPERAND(stream, x, code)	ia16_print_operand (stream, x, code)
#define PRINT_OPERAND_ADDRESS(stream, x) ia16_print_operand_address (stream, x)
#define REGISTER_PREFIX "%"
#define LOCAL_LABEL_PREFIX "."
#define IMMEDIATE_PREFIX "$"

/* This is complicated slightly because there is no 8-bit push/pop.  */
#define ASM_OUTPUT_REG_PUSH(stream, regno) \
  do { \
    fputs ("\tpush\t" REGISTER_PREFIX, stream); \
    if (regno < SI_REG) \
      {  \
	putc (reg_names[(regno) & 6][0], stream); \
	putc ('x', stream); \
      } \
    else \
      fputs (reg_names[regno], stream); \
    putc ('\n', stream); \
  } while (0);

#define ASM_OUTPUT_REG_POP(stream, regno) \
  do { \
    fputs ("\tpop\t" REGISTER_PREFIX, stream); \
    if (regno < SI_REG) \
      { \
	putc (reg_names[(regno) & 6][0], stream); \
	putc ('x', stream); \
      } \
    else \
      fputs (reg_names[regno], stream); \
    putc ('\n', stream); \
  } while (0);

/* Output of Dispatch Tables.  */
#define ASM_OUTPUT_ADDR_DIFF_ELT(stream, body, value, rel) \
	ia16_asm_output_addr_diff_elt ((stream), (body), (value), (rel))
#define ASM_OUTPUT_ADDR_VEC_ELT(stream, value) \
	ia16_asm_output_addr_vec_elt ((stream), (value))

/* Assembler Commands for Alignment.  */
#undef ASM_OUTPUT_SKIP
#define ASM_OUTPUT_SKIP(stream, nbytes) asm_fprintf (stream, "\t.skip\t%wu,0\n", nbytes)
#define ASM_OUTPUT_ALIGN(stream, power) fprintf (stream, "\t.p2align\t%u\n", power)
#define ASM_OUTPUT_ALIGN_WITH_NOP(stream, power) \
	fprintf (stream, "\t.p2align\t%u\n", power)
#define ASM_OUTPUT_MAX_SKIP_ALIGN(stream, power, max_skip) \
	fprintf (stream, "\t.p2align\t%u,,%u\n", power, max_skip)

/* Controlling Debugging Information Format  */
/* Macros Affecting All Debugging Formats  */
#undef PREFERRED_DEBUGGING_TYPE
/* Macros for SDB and DWARF Output  */
#undef DWARF2_DEBUGGING_INFO

#define REGISTER_TARGET_PRAGMAS() ia16_register_pragmas ()

/* Cross Compilation and Floating Point
 * Skipped for now.

 * Mode Switching Instructions

 * Defining target-specific uses of __attribute__
 * This is done in ia16.c .

 * Defining coprocessor specifics for MIPS targets.

 * Parameters for Precompiled Header Validity Checking

 * C++ ABI parameters
 *
 * Miscellaneous Parameters
 *
 * GAS provides the necessary workarounds for long conditional jumps to work.
 */
#define HAS_LONG_COND_BRANCH		true
#define HAS_LONG_UNCOND_BRANCH		true
#define CASE_VECTOR_MODE		Pmode
#define CASE_VECTOR_PC_RELATIVE		0
/* FIXME Not documented: caller-save.c fails if MOVE_MAX < UNITS_PER_WORD.  */
/* #define MOVE_MAX 		(UNITS_PER_WORD / (TARGET_TUNE_8BIT ? 2 : 1)) */
#define	MOVE_MAX			UNITS_PER_WORD
#define MAX_MOVE_MAX			UNITS_PER_WORD
#define TRULY_NOOP_TRUNCATION(outprec, inprec) 1
#define STORE_FLAG_VALUE		(-1)
#define Pmode				HImode
#define FUNCTION_MODE			QImode

#define ADDR_SPACE_FAR			1
/* We hack gcc/tree-ssa-loop-ivopts.c and gcc/calls.c to recognize a new
   macro TARGET_ADDR_SPACE_WEIRD_P (as), which should return true if the
   given address space breaks certain assumptions made by compiler passes.  */
#define TARGET_ADDR_SPACE_WEIRD_P(as)	((as) == ADDR_SPACE_FAR)
/* We hack gcc/c/c-parser.c to recognize TARGET_WARN_ADDR_SPACE_SYNTAX_P
   (as), which should return true if an address space name is traditionally
   considered as part of a `declarator' rather than `declaration-specifiers'
   grammar-wise.  */
#define TARGET_WARN_ADDR_SPACE_SYNTAX_P(as) ((as) == ADDR_SPACE_FAR)
/* We also hack gcc/expr.c to recognize TARGET_ADDR_SPACE_WEIRD_P (as) ---
   as well as another macro TARGET_EXPAND_WEIRD_POINTER_PLUS_EXPR (treeop0,
   treeop1, target, mode, modifier).  The latter macro should say how to
   convert a POINTER_PLUS_EXPR node into RTL, when the pointer operand
   (treeop0) belongs to a "weird" address space.  */
#define TARGET_EXPAND_WEIRD_POINTER_PLUS_EXPR \
	ia16_expand_weird_pointer_plus_expr
/* And, we hack gcc/explow.c to recognize a new macro to actually do address
   RTX -> pointer RTX and pointer RTX -> address RTX conversions for operands
   pointing into "weird" address spaces.  */
#define TARGET_ADDR_SPACE_CONVERT_WEIRD_MEMORY_ADDRESS \
	ia16_as_convert_weird_memory_address
/* To support far functions and far function pointers, we hack the C parser
   in gcc/c/c-parser.c to recognize a new macro which says we allow having
   functions in a given non-default address space.  gcc/c/c-decl.c and
   gcc/c/c-tree.h are also updated to account for the changes in the C
   parser's internal structures.  */
#define TARGET_ADDR_SPACE_MAY_HAVE_FUNCTIONS_P(as) ((as) == ADDR_SPACE_FAR)
/* Also, gcc/c/c-decl.c is updated to recognize a new macro which says
   whether a function may get its address space from its return type.  */
#define TARGET_FUNCTION_ADDR_SPACE_FROM_RETURN_TYPE_P(as) \
	TARGET_FAR_FUNCTION_IF_FAR_RETURN_TYPE

/* Which processor to tune code generation for.  These must be in sync
   with processor_target_table in ia16.c.  */

enum processor_type
{
  PROCESSOR_ANY,
  PROCESSOR_ANY_186,
  PROCESSOR_I8086,
  PROCESSOR_I8088,
  PROCESSOR_NEC_V30,
  PROCESSOR_NEC_V20,
  PROCESSOR_I80186,
  PROCESSOR_I80188,
  PROCESSOR_I80286,
  PROCESSOR_max
};

extern int ia16_features;

/* Which memory model to use.  Some memory models may not actually be
   implemented and available through the front-end.  */

enum cmodel_type
{
  CMODEL_TINY,
  CMODEL_SMALL,
  CMODEL_MEDIUM,
  CMODEL_COMPACT,
  CMODEL_LARGE,
  CMODEL_HUGE,
  CMODEL_max
};

/* General calling convention for this compilation unit --- specifically, how
   function parameters are passed and cleaned up, and how values are
   returned, for functions which do not have attributes.  Some of these may
   not actually be implemented and available through the front-end.  */

enum call_parm_cvt_type
{
  CALL_PARM_CVT_CDECL,
  CALL_PARM_CVT_STDCALL,
  CALL_PARM_CVT_REGPARMCALL,
  CALL_PARM_CVT_max
};

/* The linker will take care of this.  */
#define CTOR_LISTS_DEFINED_EXTERNALLY 1

/* No need to support Java.  */
#define TARGET_USE_JCR_SECTION 0

/* Avoid having register_tm_clones and unregister_tm_clones in crtstuff.  */
#define USE_TM_CLONE_REGISTRY 0
