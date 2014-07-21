/* GNU Compiler Collection definitions of target machine Intel 16-bit x86.
   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>

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

/* Keep this in sync with ia16.opt.  */
#define TARGET_SHIFT_IMM	(target_arch & 1)
#define TARGET_PUSH_IMM		(target_arch & 1)
#define TARGET_IMUL_IMM		(target_arch & 1)
#define TARGET_PUSHA		(target_arch & 1)
#define TARGET_ENTER_LEAVE	(target_arch & 1)
#define TARGET_SHIFT_MASKED	(target_arch & 2)
#define TARGET_AAD_IMM		(target_arch & 4)
#define TARGET_FSTSW_AX		(target_arch & 8)
#define TARGET_TUNE_8BIT	(target_tune & 8)

/* Run-time Target Specification */
#define TARGET_CPU_CPP_BUILTINS()	\
	do { builtin_define_std ("ia16"); } while (0)
#define TARGET_VERSION		fprintf (stderr, "ia16, GNU assembler syntax");

#define CAN_DEBUG_WITHOUT_FP		1

/* Storage Layout
 *
 * A unit is 8 bits, a machine word is 16 bits (two units), all little endian.
 */

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0
#define UNITS_PER_WORD 2

/* Strictly speaking, FUNCTION_BOUNDARY could be just 8 bits.  But IIRC, I
 * make an assumption somewhere else that it is at least 16 bits.  */
#define PARM_BOUNDARY		BITS_PER_WORD
#define STACK_BOUNDARY		BITS_PER_WORD
#define FUNCTION_BOUNDARY	BITS_PER_WORD
#define BIGGEST_ALIGNMENT	BITS_PER_WORD
#define STRUCTURE_SIZE_BOUNDARY	BITS_PER_UNIT
#define STRICT_ALIGNMENT	0
#define PCC_BITFIELD_TYPE_MATTERS 1

/* Layout of Source Language Data Types.  */
#define       SHORT_TYPE_SIZE 16
#define         INT_TYPE_SIZE 16
#define        LONG_TYPE_SIZE 32
#define   LONG_LONG_TYPE_SIZE 64
#define       FLOAT_TYPE_SIZE 32
#define      DOUBLE_TYPE_SIZE 64
#define LONG_DOUBLE_TYPE_SIZE 64

#define DEFAULT_SIGNED_CHAR		1
#define SIZE_TYPE			"unsigned int"
#define PTRDIFF_TYPE			"short int"
#define WCHAR_TYPE_SIZE			16

#define TARGET_VTABLE_USES_DESCRIPTORS 0

/* Register Usage
 *
 * These are the registers of the 8086 family.
 * The argument pointer register (ap/argp) is a fixed, virtual hard register.
 * We don't describe most segment registers since they are of no use to GCC.
 *
 * The order matters.  dh:dl:ah:al should be consequtive because some
 * instructions treat them as a 32-bit register.  Cl should be at one of the
 * ends or it will be impossible to fit both a shift count and a 64-bit
 * value at the same time.  Life is a lot easier if xl:xh pairs are kept
 * together.  We don't want to tie up both index (si and di) or both base w/
 * index (bx and bp) registers with a value spanning multiple hard
 * registers.
 *
 * TODO: For optimization purposes, we'd want a 64-bit value to be held in
 * registers such that the upper and lower bytes are in an 8-bit register.
 *
 * Name:  cl ch al ah dl dh bl bh si di bp es sp cc ap
 * Size:   8  8  8  8  8  8  8  8 16 16 16 16 16 cc 16
 * Index:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
 *
 * Basic Characteristics of Registers
 */
#define FIRST_PSEUDO_REGISTER 15
#define FIXED_REGISTERS \
         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 }
#define CALL_USED_REGISTERS \
         { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1 }

/* Order of Allocation of Registers.  */
/* The goal is to create opportunities for using load/store multiple
 * instructions as well as good register allocation in general.  */
#define REG_ALLOC_ORDER \
	{ 2, 3, 4, 5, 0, 1,  6, 7, 10,  9, 8, 11, 12, 13, 14 }
/*	 al ah dl dh cl ch  bl bh  bp  di si  es  sp  cc  ap */

/* How Values Fit in Registers.  */
/* FIXME: Not documented: CCmode is 32 bits.  */
/* Must not return 0 or subreg_get_info() may divide by zero.  */
/* FIXME: Handling of XFmode needs to use GET_MODE_PRECISION(). */
#define HARD_REGNO_NREGS(REGNO, MODE) \
  (MAX (ia16_hard_regno_nregs[GET_MODE_SIZE(MODE)][REGNO], 1))

/* There are more cases than those caught here, but HARD_REGNO_MODE_OK()
   forbids them. Catch multireg values ending in SI_REG. */
#define HARD_REGNO_NREGS_HAS_PADDING(REGNO, MODE) \
	((REGNO) < SI_REG && (REGNO) + GET_MODE_SIZE(MODE) == SI_REG + 2)

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
   COMPLEX_MODE_P(MODE) && (REGNO) + GET_MODE_SIZE(MODE) == SI_REG + 2 ? 0 : \
   ia16_hard_regno_nregs[GET_MODE_SIZE(MODE)][REGNO])

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
enum reg_class {	/*	 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0 */
  NO_REGS,	     /*   000000  4  2  1  4  2  1  4  2  1  4  2  1  4  2  1 */
  AL_REGS,	   /* Ral 000004  .  .  .  .  .  .  .  .  .  .  .  . al  .  . */
  AH_REGS,	   /* Rah 000010  .  .  .  .  .  .  .  .  .  .  . ah  .  .  . */
  AX_REGS,	     /* a 000014  .  .  .  .  .  .  .  .  .  .  . -- ax  .  . */
  DL_REGS,	   /* Rdl 000020  .  .  .  .  .  .  .  .  .  . dl  .  .  .  . */ 
  DH_REGS,	   /* Rdh 000040  .  .  .  .  .  .  .  .  . dh  .  .  .  .  . */
  DX_REGS,	     /* d 000060  .  .  .  .  .  .  .  .  . -- dx  .  .  .  . */
  DXAX_REGS,	     /* A 000074  .  .  .  .  .  .  .  .  . -- dx -- ax  .  . */
  BX_REGS,	     /* b 000300  .  .  .  .  .  .  . -- bx  .  .  .  .  .  . */
  BXDX_REGS,	     /* j 000360  .  .  .  .  .  .  . -- bx -- dx  .  .  .  . */
  CL_REGS,	   /* Rcl 000001  .  .  .  .  .  .  .  .  .  .  .  .  .  . cl */
  CX_REGS,	     /* c 000003  .  .  .  .  .  .  .  .  .  .  .  .  . -- cx */
  LO_QI_REGS,	     /* l 000125  .  .  .  .  .  .  .  . bl  . dl  . al  . cl */
  UP_QI_REGS,	     /* u 000252  .  .  .  .  .  .  . bh  . dh  . ah  . ch  . */
  QI_REGS,	     /* q 000377  .  .  .  .  .  .  . bh bl dh dl ah al ch cl */
  SI_REGS,	     /* S 000400  .  .  .  .  .  . si  .  .  .  .  .  .  .  . */
  QISI_REGS,	     /* k 000777  .  .  .  .  .  . si -- bx -- dx -- ax -- cx */
  DI_REGS,	     /* D 001000  .  .  .  .  . di  .  .  .  .  .  .  .  .  . */
  INDEX_REGS,	     /* x 001400  .  .  .  .  . di si  .  .  .  .  .  .  .  . */
  BP_REGS,	   /* Rbp 002000  .  .  .  . bp  .  .  .  .  .  .  .  .  .  . */
  BASE_W_INDEX_REGS, /* w 002300  .  .  .  . bp  .  . -- bx  .  .  .  .  .  . */
  BASE_REGS,	     /* B 003700  .  .  .  . bp di si -- bx  .  .  .  .  .  . */
  SEGMENT_REGS,	     /* Q 004000  .  .  . es  .  .  .  .  .  .  .  .  .  .  . */
  HI_REGS,	     /*   017400  .  . sp es bp di si  .  .  .  .  .  .  .  . */
  GENERAL_REGS,	     /* r 013777  .  . sp  . bp di si bh bl dh dl ah al ch cl */
  SEG_GENERAL_REGS,  /* T 017777  .  . sp es bp di si bh bl dh dl ah al ch cl */
  ALL_REGS,	     /*   077777 ap cc sp es bp di si bh bl dh dl ah al ch cl */
  LIM_REG_CLASSES
};
#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define REG_CLASS_NAMES \
{ "NO_REGS", "AL_REGS", "AH_REGS", "AX_REGS", "DL_REGS", "DH_REGS", \
  "DX_REGS", "DXAX_REGS", "BX_REGS", "BXDX_REGS", "CL_REGS", \
  "CX_REGS", "LO_QI_REGS", "UP_QI_REGS", "QI_REGS", \
  "SI_REGS", "QISI_REGS", "DI_REGS", "INDEX_REGS", \
  "BP_REGS", "BASE_W_INDEX_REGS", "BASE_REGS", "SEGMENT_REGS", "HI_REGS", \
  "GENERAL_REGS", "SEG_GENERAL_REGS", "ALL_REGS" }

#define REG_CLASS_CONTENTS {	 /* 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0 */ \
  { 000000 }, /* NO_REGS,	     4  2  1  4  2  1  4  2  1  4  2  1  4  2  1 */ \
  { 000004 }, /* AL_REGS,	 Ral .  .  .  .  .  .  .  .  .  .  .  . al  .  . */ \
  { 000010 }, /* AH_REGS,	 Rah .  .  .  .  .  .  .  .  .  .  . ah  .  .  . */ \
  { 000014 }, /* AX_REGS,	   a .  .  .  .  .  .  .  .  .  .  . -- ax  .  . */ \
  { 000020 }, /* DL_REGS,	 Rdl .  .  .  .  .  .  .  .  .  . dl  .  .  .  . */ \
  { 000040 }, /* DH_REGS,	 Rdh .  .  .  .  .  .  .  .  . dh  .  .  .  .  . */ \
  { 000060 }, /* DX_REGS,	   d .  .  .  .  .  .  .  .  . -- dx  .  .  .  . */ \
  { 000074 }, /* DXAX_REGS,	   A .  .  .  .  .  .  .  .  . -- dx -- ax  .  . */ \
  { 000300 }, /* BX_REGS,	   b .  .  .  .  .  .  . -- bx  .  .  .  .  .  . */ \
  { 000360 }, /* BXDX_REGS,	   j .  .  .  .  .  .  . -- bx -- dx  .  .  .  . */ \
  { 000001 }, /* CL_REGS,	 Rcl .  .  .  .  .  .  .  .  .  .  .  .  .  . cl */ \
  { 000003 }, /* CX_REGS,	   c .  .  .  .  .  .  .  .  .  .  .  .  . -- cx */ \
  { 000125 }, /* LO_QI_REGS,	   l .  .  .  .  .  .  .  . bl  . dl  . al  . cl */ \
  { 000252 }, /* UP_QI_REGS,	   u .  .  .  .  .  .  . bh  . dh  . ah  . ch  . */ \
  { 000377 }, /* QI_REGS,	   q .  .  .  .  .  .  . bh bl dh dl ah al ch cl */ \
  { 000400 }, /* SI_REGS,	   S .  .  .  .  .  . si  .  .  .  .  .  .  .  . */ \
  { 000777 }, /* QISI_REGS,	   k .  .  .  .  .  . si -- bx -- dx -- ax -- cx */ \
  { 001000 }, /* DI_REGS,	   D .  .  .  .  . di  .  .  .  .  .  .  .  .  . */ \
  { 001400 }, /* INDEX_REGS,	   x .  .  .  .  . di si  .  .  .  .  .  .  .  . */ \
  { 002000 }, /* BP_REGS,	 Rbp .  .  .  . bp  .  .  .  .  .  .  .  .  .  . */ \
  { 002300 }, /* BASE_W_INDEX_REGS w .  .  .  . bp  .  . -- bx  .  .  .  .  .  . */ \
  { 003700 }, /* BASE_REGS,	   B .  .  .  . bp di si -- bx  .  .  .  .  .  . */ \
  { 004000 }, /* SEGMENT_REGS,	   Q .  .  . es  .  .  .  .  .  .  .  .  .  .  . */ \
  { 017400 }, /* HI_REGS,	     .  . sp es bp di si  .  .  .  .  .  .  .  . */ \
  { 013777 }, /* GENERAL_REGS,	   r .  . sp  . bp di si bh bl dh dl ah al ch cl */ \
  { 017777 }, /* SEG_GENERAL_REGS, T .  . sp es bp di si bh bl dh dl ah al ch cl */ \
  { 077777 }, /* ALL_REGS,	    ap cc sp es bp di si bh bl dh dl ah al ch cl */ \
}

#define REGNO_REG_CLASS(regno) ia16_regno_class[regno]

/* We use INDEX_REGS for BASE_REG_REGS and BASE_W_INDEX_REGS for INDEX_REGS
 * because this enables GCC to have use two base registers for base+index
 * addressing when not eliminating the frame pointer.  This improves i80286
 * __gcc_bcmp() a lot (or it used to).
 */
#define MODE_BASE_REG_CLASS(mode)	((mode) == PQImode ? GENERAL_REGS : BASE_REGS)
#define MODE_BASE_REG_REG_CLASS(mode)	((mode) == PQImode ? NO_REGS : INDEX_REGS)
#define INDEX_REG_CLASS			BASE_W_INDEX_REGS

/* FIXME: Documentation:
 * It is unclear if these definitions should be guarded by REG_OK_STRICT or not.  */
#define REGNO_MODE_OK_FOR_BASE_P(num, mode)	((num) < FIRST_PSEUDO_REGISTER && \
	TEST_HARD_REG_BIT (reg_class_contents[MODE_BASE_REG_CLASS (mode)], (num)))
#define REGNO_MODE_OK_FOR_REG_BASE_P(num, mode)	((num) < FIRST_PSEUDO_REGISTER && \
	TEST_HARD_REG_BIT (reg_class_contents[MODE_BASE_REG_REG_CLASS (mode)], (num)))
#define REGNO_OK_FOR_INDEX_P(num)		((num) < FIRST_PSEUDO_REGISTER && \
	TEST_HARD_REG_BIT (reg_class_contents[INDEX_REG_CLASS], (num)))

#define PREFERRED_RELOAD_CLASS(X,CLASS) CLASS

/* IA16 easily qualifies as having small register classes.  But I
   have yet to see a case where SMALL_REGISTER_CLASSES made a
   reload failure go away, so turn it off.
   However, compiling the testcase calltest.c will fail if we don't
   turn this on _or_ make this case caught by
   CLASS_LIKELY_SPILLED_P (REGNO_REG_CLASS (A_REG)).
*/
#define SMALL_REGISTER_CLASSES	0

#define CLASS_LIKELY_SPILLED_P(c)	\
  ((c) == AX_REGS || (c) == DX_REGS || reg_class_size[c] == 1)

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

#define HARD_REGNO_NREGS_OVERRIDES_CLASS_MAX_NREGS

/* HI_REGS cannot change mode to QImode.  We cannot change mode to a
 * larger mode without increasing the number of hard regs used.
 * TODO: This will change when x87 FPUs are supported.  */
#define CANNOT_CHANGE_MODE_CLASS(FROM, TO, CLASS)	\
	(GET_MODE_SIZE(TO) > GET_MODE_SIZE(FROM)	\
	 || ((TO) == QImode && reg_classes_intersect_p (HI_REGS, (CLASS))))

/* Stack Layout and Calling Conventions
 * Basic Stack Layout
 */

#define STACK_GROWS_DOWNWARD
#define FRAME_GROWS_DOWNWARD 1
#undef  ARGS_GROW_DOWNWARD
#define STARTING_FRAME_OFFSET 0
#define FIRST_PARM_OFFSET(fundecl) ia16_first_parm_offset ()

/* Taken from i386/i386.h.  */
/* TODO: Check that this is still correct.  */
#define RETURN_ADDR_RTX(COUNT, FRAME)						 \
	((COUNT) == 0								 \
	 ? gen_rtx_MEM (Pmode, plus_constant (arg_pointer_rtx, -UNITS_PER_WORD)) \
	 : gen_rtx_MEM (Pmode, plus_constant (FRAME, UNITS_PER_WORD)))

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
#define FRAME_POINTER_REQUIRED 0

/* INITIAL_FRAME_POINTER_OFFSET(depth_var) */
/* FIXME Documentation bug: depth-var is invalid as a macro parameter.  */
/* FIXME Documentation: The offset is in units and is positive if
   STACK_GROWS_UPWARD.  */

#define ELIMINABLE_REGS \
	{	{ ARG_POINTER_REGNUM, STACK_POINTER_REGNUM },	\
		{ ARG_POINTER_REGNUM, FRAME_POINTER_REGNUM },	\
		{ FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM }	}
#define CAN_ELIMINATE(from_reg, to_reg)	1
#define INITIAL_ELIMINATION_OFFSET(from_reg, to_reg, offset_var) \
	offset_var = ia16_initial_elimination_offset (from_reg, to_reg)

/* Passing Function Arguments on the Stack */
#define PUSH_ARGS		1
#define PUSH_ARGS_REVERSED	1
#define PUSH_ROUNDING(BYTES)	(((BYTES) + 1) & ~1)

/* TODO: Return 1 for functions which take a fixed number of arguments.  */
/* FIXME: Docs say "stack-size" which isn't allowed in a macro parameter.  */
#define RETURN_POPS_ARGS(fundecl, funtype, stack_size) 0

/* Passing Arguments in Registers */
/* TODO: Allow arguments to be passed in registers.  */
#define FUNCTION_ARG(cum, mode, type, named)	0
#define FUNCTION_ARG_REGNO_P(regno)		0
#define CUMULATIVE_ARGS				int

/* The next two were modified from m68k/m68k.h.  */
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, INDIRECT, N_NAMED_ARGS) \
	((CUM) = 0)

#define FUNCTION_ARG_ADVANCE(CUM, MODE, TYPE, NAMED)	\
	((CUM) += ((MODE) != BLKmode			\
		? (GET_MODE_SIZE (MODE) + 1) & ~1	\
		: (int_size_in_bytes (TYPE) + 1) & ~1))

/* How Scalar Function Values Are Returned */
#define LIBCALL_VALUE(mode)	\
  (HARD_REGNO_MODE_OK (A_REG, mode) ? gen_rtx_REG (mode, A_REG) : NULL_RTX)
#define FUNCTION_VALUE_REGNO_P(N)	((N) == A_REG)

/* How Large Values Are Returned */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Caller-Saves Register Allocation */
/* defaults.  */

/* Function Entry and Exit */
/* Thunks support is missing.  */
/* Stack adjustment at function exit isn't needed with a frame pointer.  */
#define EXIT_IGNORE_STACK (!current_function_calls_alloca)

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
#define INITIALIZE_TRAMPOLINE(addr, fnaddr, static_chain) ia16_initialize_trampoline (addr, fnaddr, static_chain)

/* Implicit Calls to Library Routines */
/* defaults.  */

/* Addressing Modes */
#define CONSTANT_ADDRESS_P(x)	CONSTANT_P (x)
#define MAX_REGS_PER_ADDRESS	2

/* This is silly.  I've already defined BASE_REG_CLASS, BASE_REG_REG_CLASS,
 * and INDEX_REG_CLASS, so this is duplicated from REG_CLASS_CONTENTS.
 * When not REG_OK_STRICT, pseudo registers and virtual hard registers must
 * be accepted as well as real hard registers of the required class.
 * FIXME: Documentation: It is unclear if you need true_regnum() or not.
 * true_regnum() will crash if reg_renumber[] hasn't been allocated yet.
 */
#ifdef REG_OK_STRICT

#define REG_MODE_OK_FOR_BASE_P(x, mode)		(REGNO_MODE_OK_FOR_BASE_P (REGNO (x), mode))
#define REG_MODE_OK_FOR_REG_BASE_P(x, mode)	(REGNO_MODE_OK_FOR_REG_BASE_P (REGNO (x), mode))
#define REG_OK_FOR_INDEX_P(x)			(REGNO_OK_FOR_INDEX_P (REGNO (x)))

#else

#define REG_MODE_OK_FOR_BASE_P(x, mode)		\
  (REGNO (x) > LAST_HARD_REG || REGNO_MODE_OK_FOR_BASE_P (REGNO (x), mode))

#define REG_MODE_OK_FOR_REG_BASE_P(x, mode)	\
  (REGNO (x) > LAST_HARD_REG || REGNO_MODE_OK_FOR_REG_BASE_P (REGNO (x), mode))

#define REG_OK_FOR_INDEX_P(x)			\
  (REGNO (x) > LAST_HARD_REG || REGNO_OK_FOR_INDEX_P (REGNO (x)))

#endif

/* PQImode is used for the function addresses in call insns, so we can
 * recognize them here.  */
#define GO_IF_LEGITIMATE_ADDRESS(xmode, x, label)			\
{ rtx y, r1, r2;							\
  enum machine_mode ymode;						\
									\
  if ((xmode) == PQImode)						\
    if (CONSTANT_P (x) || (REG_P (x) && REG_MODE_OK_FOR_BASE_P (x, xmode))) \
      goto label;							\
    else								\
      {									\
        y = XEXP (x, 0);						\
	ymode = GET_MODE (y);						\
      }									\
  else									\
    {									\
      y = x;								\
      ymode = xmode;							\
    }									\
  if (ia16_parse_address (y, &r1, &r2, NULL))				\
    if (r1)								\
      if (r2)								\
	if ( (REG_OK_FOR_INDEX_P (r1) && REG_MODE_OK_FOR_REG_BASE_P (r2, ymode)) \
	  || (REG_OK_FOR_INDEX_P (r2) && REG_MODE_OK_FOR_REG_BASE_P (r1, ymode))) \
	  goto label;							\
	else								\
	  ;								\
      else								\
	{								\
	  if (REG_MODE_OK_FOR_BASE_P (r1, ymode))			\
	    goto label;							\
	}								\
    else								\
      goto label;							\
}

#define GO_IF_MODE_DEPENDENT_ADDRESS(addr, label)	{}
#define LEGITIMATE_CONSTANT_P(x)		1

/* Condition Code Status.  */
extern GTY(()) rtx ia16_cmp_op0;
extern GTY(()) rtx ia16_cmp_op1;

/* Un-canonicalize op0 == -op1 as op0 + op1 == 0. Likewise with !=.  */
#define CANONICALIZE_COMPARISON(code, op0 ,op1)	\
{	\
  if ((code == EQ || code == NE)	\
      && GET_CODE (op1) == NEG)		\
    {					\
      op0 = gen_rtx_PLUS (GET_MODE (op0), op0, XEXP (op1, 0));	\
      op1 = const0_rtx;			\
    }					\
}
#define SELECT_CC_MODE(op, x, y)	ia16_select_cc_mode (op, x, y, false)

#define REVERSIBLE_CC_MODE(mode)	1

/* Describing Relative Costs of Operations.
 * It is, on average, slightly cheaper to copy to/from memory with al/ax
 * than with other registers, because a faster, immediate address
 * instruction exists.  Segments registers, if supported, would also be
 * cheaper to copy to/from memory.  Costs are based on timings for the 80186
 * and much too high for the 80286.  Discourage regclass.c from using
 * ALL_REGS as alternate class.
 * TODO: Now that we have ia16_costs, use it.  But that needs
 * stuff in ia16-protos.h.
 */
#define REGISTER_MOVE_COST(mode, from, to) \
  (COSTS_N_INSNS (from == ALL_REGS || to == ALL_REGS ? 16000 : 2))

#define MEMORY_MOVE_COST(mode, class, in) \
  ((TARGET_TUNE_8BIT ? (GET_MODE_SIZE (mode) - 1) * COSTS_N_INSNS (4) : 0) + \
  ((class) == AX_REGS ? (in ? COSTS_N_INSNS (8) : COSTS_N_INSNS (11)) : \
  (in ? COSTS_N_INSNS (9) : COSTS_N_INSNS (12))))

/* A taken branch costs 13 cycles and a not taken branch costs 4 cycles.
 * Add to that 4 (or 8) cycles to fetch the branch instruction (two bytes)
 * itself.
 */
#define BRANCH_COST  (TARGET_TUNE_8BIT ? COSTS_N_INSNS (8) : COSTS_N_INSNS (4) \
		      + COSTS_N_INSNS (8))
#define SLOW_BYTE_ACCESS	(TARGET_TUNE_8BIT ? 0 : 1)

/* These probably need some tweaking.  Leave the defaults for now.
#define MOVE_RATIO
#define CLEAR_RATIO
*/

/* Loading a function address into a register takes 3 bytes and 4 cycles
 * (i80186) and saves at most 1 byte and 2 cycles (i80186) per call.  So don't
 * waste a precious register on this. TODO: Why does this have no effect?
 */
#define NO_FUNCTION_CSE

/* Adjusting the Instruction Scheduler.
   #define TARGET_SCHED_ISSUE_RATE() MAX_DFA_ISSUE_RATE
*/
/* Dividing the Output into Sections (Texts, Data, ...).  */
#define TEXT_SECTION_ASM_OP	"\t.text"
#define DATA_SECTION_ASM_OP	"\t.data"
#define BSS_SECTION_ASM_OP	"\t.bss"
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
#define GLOBAL_ASM_OP	"\t.global\t"

/* This is obsoleted by TARGET_ASM_EXTERNAL_LIBCALL and wrong in elfos.h.  */
#undef ASM_OUTPUT_EXTERNAL_LIBCALL

/* How Initialization Functions Are Handled.  */
/* skipped for now.*/

/* Output of Assembler Instructions.  */

#define REGISTER_NAMES { "c", "ch", "a", "ah", "d", "dh", "b", "bh", \
                         "si", "di", "bp", "es", "sp", "cc", "argp" }
#define ADDITIONAL_REGISTER_NAMES \
	{ { "cl", 0 }, {"al", 2 }, { "dl", 4 }, { "bl", 6 }, \
	  { "cx", 0 }, {"ax", 2 }, { "dx", 4 }, { "bx", 6 } }
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
	fprintf (stream, "\t.word\t.%u-.%u\n", value, rel)

/* Assembler Commands for Exception Regions.  */
#undef ASM_OUTPUT_SKIP
#define ASM_OUTPUT_SKIP(stream, nbytes) asm_fprintf (stream, "\t.skip\t%wu,0\n", nbytes)
#define ASM_OUTPUT_ALIGN(stream, power) fprintf (stream, "\t.p2align\t%u\n", power)
#define ASM_OUTPUT_ALIGN_WITH_NOP(stream, power) \
	fprintf (stream, "\t.p2align\t%u\n", power)
#define ASM_OUTPUT_MAX_SKIP_ALIGN(stream, power, max_skip) \
	fprintf (stream, "\t.p2align\t%u,,%u\n", power, max_skip)

/* Controlling Debugging Information Format
 * Skipped - my programs have no bugs :-)

 * Cross Compilation and Floating Point
 * Skipped for now.

 * Mode Switching Instructions

 * Defining target-specific uses of __attribute__
 * Maybe __attribute("far")__ some day.

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
#define CASE_VECTOR_PC_RELATIVE		1
/* FIXME Not documented: caller-save.c fails if MOVE_MAX < UNITS_PER_WORD.  */
/* #define MOVE_MAX 		(UNITS_PER_WORD / (TARGET_TUNE_8BIT ? 2 : 1)) */
#define	MOVE_MAX			UNITS_PER_WORD
#define MAX_MOVE_MAX			UNITS_PER_WORD
#define TRULY_NOOP_TRUNCATION(outprec, inprec) 1
#define STORE_FLAG_VALUE		(-1)
#define Pmode				HImode
#define FUNCTION_MODE			PQImode
