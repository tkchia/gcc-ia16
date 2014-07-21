/* Subroutines used during code generation for Intel 16-bit x86.
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

/* I have no or little idea how many and which of these headers files I need
 * to include.  So don't look here if you don't have a clue either.
 *
 * FIXME: Docs say config.h includes tm.h, but it doesn't.
 */
#define REG_OK_STRICT
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "expr.h"
#include "optabs.h"
#include "df.h"

/* The Global targetm Variable.  */
/* Part 1.  Part 2 is at the end of this file.  */

#include "target.h"
#include "target-def.h"

#include "regs.h"
#include "hard-reg-set.h"
#include "expr.h"
#include "toplev.h"
#include "insn-constants.h"
#include "output.h"
#include "tm_p.h"
#include <stdio.h>

/* Run-time Target Specification */
/* Part 1, see section "Describing Relative Costs of Operations" for the
 * second part.  */
#undef  TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION ia16_handle_option

/* Storage Layout.  */
#define TARGET_MS_BITFIELD_LAYOUT_P	hook_bool_tree_false
#define TARGET_PROMOTE_FUNCTION_ARGS	hook_bool_tree_false
#define TARGET_PROMOTE_FUNCTION_RETURN	hook_bool_tree_false

/* Layout of Source Language Data Types */
#undef  TARGET_DEFAULT_SHORT_ENUMS
#define TARGET_DEFAULT_SHORT_ENUMS	hook_bool_void_true

/* How Values Fit in Registers */
/* This is complex enough to warrant a table.
 * Only 8-bit registers can hold QImode values. TODO: Lift this restriction.
 * Multibyte values are forced to start in even registers because the hw
 * makes this much easier and faster and some insn patterns may rely on it.
 * Don't put 8-byte values in bx because it would take all BASE_REGS.
 * Don't put 8-byte values in bx or dx because subreg_get_info can't handle it.
 * find_valid_class() fails if we don't take the ending register into account.
 * subreg_get_info() dies on libssp/gets-chk.c if !H_R_M_O (SP_REG, HImode).
 * Disallow register size changes unless HARD_REGNO_NREGS_HAS_PADDING.
 * CCmode is 4 bytes.
 */
unsigned char ia16_hard_regno_nregs[17][FIRST_PSEUDO_REGISTER] =
{
/* size     cl  ch  al  ah  dl  dh  bl  bh  si  di  bp  es  sp  cc, ap */
/*  0 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  1 */  {  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0 },
/*  2 */  {  2,  0,  2,  0,  2,  0,  2,  0,  1,  1,  1,  1,  1,  0,  1 },
/*  3 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  4 */  {  4,  0,  4,  0,  4,  0,  3,  0,  2,  2,  2,  2,  0,  1,  0 },
/*  5 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  6 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  7 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/*  8 */  {  8,  0,  7,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0,  0 },
/*  9 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 10 */  {  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 11 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 12 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 13 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 14 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 15 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
/* 16 */  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
};

/* Register Classes.  */
unsigned char ia16_regno_class[FIRST_PSEUDO_REGISTER] = {
	/*  0 cl */ CL_REGS,
	/*  1 ch */ CX_REGS,
	/*  2 al */ AL_REGS,
	/*  3 ah */ AH_REGS,
	/*  4 dl */ DL_REGS,
	/*  5 dh */ DH_REGS,
	/*  6 bl */ BX_REGS,
	/*  7 bh */ BX_REGS,
	/*  8 si */ SI_REGS,
	/*  9 di */ DI_REGS,
	/* 10 bp */ BP_REGS,
	/* 11 es */ SEGMENT_REGS,
	/* 12 sp */ HI_REGS,
	/* 13 cc */ ALL_REGS,
	/* 14 ap */ ALL_REGS,
};

#undef  TARGET_SECONDARY_RELOAD
#define TARGET_SECONDARY_RELOAD ia16_secondary_reload

/* Handle loading a constant into a segment register.  Handle loading a frame
 * pointer offset into a segment register.  This is also where copies between
 * segment registers should be handled, if needed.  All of this needs an
 * intermediate register of class GENERAL_REGS.  */
static enum reg_class
ia16_secondary_reload (bool in_p, rtx x, enum reg_class reload_class,
		       enum machine_mode reload_mode ATTRIBUTE_UNUSED,
		       secondary_reload_info *sri ATTRIBUTE_UNUSED)
{
  if (in_p
      && (reload_class == SEG_GENERAL_REGS
	  || reload_class == SEGMENT_REGS)
      && (CONSTANT_P (x) || PLUS == GET_CODE (x)))
    {
      return (GENERAL_REGS);
    }
  return (NO_REGS);
}

/* Returns non-zero if register r must be saved by a function, zero if not.  */
/* Always returns zero for upper halves of 16-bit registers
 * (i.e. ah, dh, bh or ch).  */
int ia16_save_reg_p (unsigned int r)
{
	if (!TEST_HARD_REG_BIT (reg_class_contents[QI_REGS], r))
		return (df_regs_ever_live_p (r) && !call_used_regs[r]);
	if (TEST_HARD_REG_BIT (reg_class_contents[UP_QI_REGS], r))
		return (0);
	return ((df_regs_ever_live_p (r + 0) && !call_used_regs[r + 0]) ||
	        (df_regs_ever_live_p (r + 1) && !call_used_regs[r + 1]));
}

/* Basic Stack Layout */
/* Right after the function prologue, before elimination, we have:
   argument N
   argument N - 1
   ...
   argument 1
   return address		<-- argument pointer
   saved reg 1
   saved reg 2
   ...
   saved reg N
   saved reg bp?		<-- frame pointer
   local variables start
   local variables end		<-- stack pointer
   function outgoing arguments
*/

/* Calculates the offset from the argument pointer to the first argument.
 * When a frame pointer is needed and bp must be saved, it is saved before the
 * frame is created.  This increases the offset to the parameters by 2 bytes.
 * A __far__ call (not yet implemented) would add another 2 bytes.
 * FIXME: Docs: This is called before register allocation.
 */
HOST_WIDE_INT ia16_first_parm_offset (void)
{
	/* Start off with 2 bytes to skip over the saved pc register.  */
	HOST_WIDE_INT offset = GET_MODE_SIZE (Pmode);

	return (offset);
}

/* Eliminating Frame Pointer and Arg Pointer */
/* Calculates the difference between the argument pointer and the frame
 * pointer immediately after the function prologue.  This should be kept int
 * sync with the prologue pattern.
 */
static HOST_WIDE_INT
ia16_initial_arg_pointer_offset (void)
{
  HOST_WIDE_INT offset = 0;
  unsigned int i;

  /* Add two bytes for each register saved.  */
  for (i = 0; i < BP_REG; i ++)
    {
      if (ia16_save_reg_p (i))
	{
	  offset += GET_MODE_SIZE (HImode);
	}
    }
  if (ia16_save_reg_p (ES_REG))
    offset += GET_MODE_SIZE (HImode);

  /* Add two bytes if we saved the bp register.  */
  if ((frame_pointer_needed && !call_used_regs[BP_REG])
      || ia16_save_reg_p (BP_REG))
    {
      offset += GET_MODE_SIZE (HImode);
    }
  return (offset);
}

/* Calculates the difference between the frame pointer and the stack pointer
 * values immediately after the function prologue.  This should be kept in sync
 * with the prologue pattern.
 */
HOST_WIDE_INT
ia16_initial_frame_pointer_offset (void)
{
  HOST_WIDE_INT offset;
	
  offset = get_frame_size ()
	 + current_function_outgoing_args_size
	 + current_function_pretend_args_size;

  return (offset);
}

HOST_WIDE_INT
ia16_initial_elimination_offset (unsigned int from, unsigned int to)
{
  if (ARG_POINTER_REGNUM == from && FRAME_POINTER_REGNUM == to)
    return (ia16_initial_arg_pointer_offset ());

  if (FRAME_POINTER_REGNUM == from && STACK_POINTER_REGNUM == to)
    return (ia16_initial_frame_pointer_offset ());

  if (ARG_POINTER_REGNUM == from && STACK_POINTER_REGNUM == to)
    return (ia16_initial_arg_pointer_offset ()
	  + ia16_initial_frame_pointer_offset ());

  gcc_unreachable ();
}

/* Passing Arguments in Registers */
#undef  TARGET_VECTOR_MODE_SUPPORTED_P
#define TARGET_VECTOR_MODE_SUPPORTED_P ia16_vector_mode_supported_p
static bool
ia16_vector_mode_supported_p (enum machine_mode mode)
{
  return (mode == V2QImode);
}

/* How Scalar Function Values Are Returned */
#undef  TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE ia16_function_value
static rtx
ia16_function_value (tree ret_type, tree fn_decl_or_type ATTRIBUTE_UNUSED,
		     bool outgoing ATTRIBUTE_UNUSED)
{
  return gen_rtx_REG (TYPE_MODE (ret_type), A_REG);
}

/* How Large Values Are Returned */
/* FIXME Documentation: This is for scalar values as well.  */
#undef  TARGET_RETURN_IN_MEMORY
#define TARGET_RETURN_IN_MEMORY ia16_return_in_memory
static bool
ia16_return_in_memory (tree type, tree fntype ATTRIBUTE_UNUSED)
{
  /* Return in memory if it's larger than 4 bytes or BLKmode.
   * TODO: Increase this to 8 bytes or so.  Doing so requires more call
   * used registers or requires the prologue and epilogue patterns to not
   * touch the return value registers.  */
   return (TYPE_MODE (type) == BLKmode || int_size_in_bytes (type) > 4);
}

/* Condition Code Status */
/* Return the "smallest" usable comparison mode for the given comparison
 * operator OP and operands X and Y.  BRANCH is true if we are optimizing for
 * a branch instruction.  */
enum machine_mode
ia16_select_cc_mode (enum rtx_code op, rtx x, rtx y,
		     bool branch ATTRIBUTE_UNUSED)
{
  switch (op)
    {
    case EQ:
    case NE:
      /* TODO: Explain why we check nonimmediate_operand(x) here.  */
      if ((y == const0_rtx || y == constm1_rtx)
	   && nonimmediate_operand (x, GET_MODE (x)))
	return y == const0_rtx ? CCZ_Cmode : CCZ_NCmode;
      else
	return CCZmode;

    case LT:
    case GE:
      /* We don't need to set the overflow flag when comparing against zero.
	 The add/sub instructions don't set the overflow flag usefully.  */
      if (y == const0_rtx)
	return CCSmode;
      else
	return CCSOmode;

    case LE:
    case GT:
      /* We have no jump instructions for CCSZmode, so always use CCSOZmode.  */
      return CCSOZmode;

    case LTU:
    case GEU:
      return CCCmode;

    case LEU:
    case GTU:
      /* Detect overflow checks.  */
      if (GET_CODE (x) == MINUS
	  && rtx_equal_p (XEXP (x, 0), y))
	return CCCZ_NCmode;
      /* Comparison against immediates don't need to set the zero flag.  */
      return immediate_operand (y, GET_MODE (y)) ? CCCZ_Cmode : CCCZmode;

    default:
    return CCmode;
    }
}
/* Comparison insns are not emitted until the usage of the comparison result is
 * know.  The comparison operands are saved here.  */
rtx ia16_cmp_op0, ia16_cmp_op1;

/* Emit a comparison instruction for the comparison operator OP and the two
 * operands X and Y. If BRANCH is true, optimize for a branch instruction. 
 * Return the register which holds the comparison result.  */
rtx
ia16_gen_compare_reg (enum rtx_code op, rtx x, rtx y, bool branch)
{
  enum machine_mode mode = ia16_select_cc_mode (op, x, y, branch);
  rtx cc_reg = gen_rtx_REG (mode, CC_REG);

  if (MEM_P (x) && MEM_P (y))
    y = force_reg (GET_MODE (y), y);
  emit_insn (gen_rtx_SET (VOIDmode, cc_reg, gen_rtx_COMPARE (mode, x, y)));
  return cc_reg;
}

#undef  TARGET_FIXED_CONDITION_CODE_REGS
#define TARGET_FIXED_CONDITION_CODE_REGS ia16_fixed_condition_code_regs

/* CC_REG is a fixed condition code register.  */
static bool
ia16_fixed_condition_code_regs (unsigned int *reg1, unsigned int *reg2)
{
	*reg1 = CC_REG;
	*reg2 = INVALID_REGNUM;
	return (true);
}

#undef  TARGET_CC_MODES_COMPATIBLE
#define TARGET_CC_MODES_COMPATIBLE ia16_cc_modes_compatible

/* TODO: Convert this into a table.  */
enum machine_mode
ia16_cc_modes_compatible (enum machine_mode mode1, enum machine_mode mode2)
{
  switch (mode1)
    {

      case CCSCmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	      return (CCSCZmode);
	    case CCSCmode:
	    case CCCmode:
	      return (CCSCmode);
	    default:
	      return (CCmode);
	  }

      case CCSOZmode:
	switch (mode2)
	  {
	    case CCSOZmode:
	    case CCSOmode:
	    case CCSZmode:
	    case CCZmode:
	      return (CCSOZmode);
	    default:
	      return (CCmode);
	  }

      case CCSOmode:
	switch (mode2)
	  {
	    case CCSOZmode:
	    case CCSOmode:
	      return (mode2);
	    case CCSmode:
	      return (CCSOmode);
	    default:
	      return (CCmode);
	  }

      case CCSmode:
	switch (mode2)
	  {
	    case CCSZmode:
	    case CCSOmode:
	    case CCSOZmode:
	      return (mode2);
	    default:
	      return (CCmode);
	  }

      case CCCZmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	      return (CCSCZmode);
	    case CCCZmode:
	    case CCZmode:
	    case CCCmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCZmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	    case CCSOZmode:
	    case CCSZmode:
	    case CCCZmode:
	    case CCZmode:
	      return (mode2);
	    case CCSOmode:
	      return (CCSOZmode);
	    case CCCmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCCmode:
	switch (mode2)
	  {
	    case CCSCZmode:
	    case CCSCmode:
	    case CCCZmode:
	    case CCCmode:
	      return (mode2);
	    case CCZmode:
	      return (CCCZmode);
	    default:
	      return (CCmode);
	  }

      case CCmode:
      default:
	return (CCmode);
    }
    gcc_unreachable ();
}

/* Describing Relative Costs of Operations */

/* This is modelled after the i386 port.
   Often, there are different costs reg,reg->reg, mem,reg->reg
   and reg,mem->mem.  The cost of memory address calculation, calculated using
   ea_calc (also for lea), will be added except for xlat.
*/
struct processor_costs {
  const int byte_fetch;		/* cost of fetching an instruction byte.  */
  int (*const ea_calc)(rtx r1, rtx r2, rtx c);
				/* cost of calculating an address.  */
  const int move;		/* cost of reg-reg mov instruction.  */
  const int imm_load[2];	/* cost of loading imm (QI, HI) to register.  */
  const int imm_store[2];	/* cost of storing imm (QI, HI) to memory.  */
  const int int_load[2];	/* cost of loading integer register
				   from memory in QImode and HImode.  */
  const int int_store[2];	/* cost of storing integer register
				   to memory in QImode and HImode */
  const int move_ratio;		/* The threshold of number of scalar
				   memory-to-memory move insns.  */
  const int add[3];		/* cost of an add instruction */
  const int add_imm[2];		/* cost of an add imm (reg, mem) instruction */
  const int inc_dec[2];
  const int lea;		/* cost of a lea instruction */
  const int cmp[3];		/* cost of a cmp instruction */
  const int cmp_imm[2];		/* cost of a cmp imm (reg, mem) instruction */
  const int shift_1bit[2];	/* single bit shift/rotate cost */
  const int shift_start[2];	/* cost of starting shift/rotate (reg, mem) */
  const int shift_bit;		/* cost of shift/rotate per bit */
  const int s_mult_init[2][2];	/* cost of starting a signed multiply in
				   { QI { r, m }, HI { r, m } } */
  const int u_mult_init[2][2];	/* cost of starting an unsigned multiply in
				   { QI { r, m }, HI { r, m } } */
  const int mult_imm_init[2];	/* cost of starting imul reg/mem.  */
  const int mult_bit;		/* cost of multiply per each bit set */
  const int s_divide[2][2];	/* cost of a signed divide/mod
				   in QImode, HImode */
  const int u_divide[2][2];	/* cost of an unsigned divide.  */
  const int sign_extend[2];	/* The cost of sign extension (QI, HI).  */
  const int xlat;		/* cost of an xlat instruction.  */
  const int call[3];		/* cost of call {imm, reg, mem}.  */
  const int fp_move;		/* cost of reg,reg fld/fst */
  const int fp_load[3];		/* cost of loading FP register
				   in SFmode, DFmode and XFmode */
  const int fp_store[3];	/* cost of storing FP register
				   in SFmode, DFmode and XFmode */
  const int branch_cost;	/* Default value for BRANCH_COST.  */
  const int fadd;		/* cost of FADD and FSUB instructions.  */
  const int fmul;		/* cost of FMUL instruction.  */
  const int fdiv;		/* cost of FDIV instruction.  */
  const int fabs;		/* cost of FABS instruction.  */
  const int fchs;		/* cost of FCHS instruction.  */
  const int fsqrt;		/* cost of FSQRT instruction.  */
};

extern const  struct processor_costs *ia16_costs;
static struct processor_costs  ia16_size_costs;

#define IA16_COST(F)	\
	(MAX (ia16_costs->F, ia16_costs->byte_fetch * ia16_size_costs.F))

#define C(x)	COSTS_N_INSNS(x)

/* Return the cost of a constant X in mode MODE in an OUTER_CODE rtx.  */
/* TODO: Accurate costs of vector constants.  */
static int
ia16_constant_cost (rtx x, enum machine_mode mode, int outer_code)
{
  int n;

  if (!CONST_INT_P (x))
    return (C (ia16_costs->byte_fetch * GET_MODE_SIZE (Pmode)));

  n = INTVAL (x);
  if (mode == VOIDmode)
    mode = Pmode;

  /* Some instructions have implicit constants.  */
  if (n == 1)
    switch (outer_code)
      {
	case PLUS:
	case MINUS:
	case ASHIFT:
	case ASHIFTRT:
	case LSHIFTRT:
	case ROTATE:
	case ROTATERT:
	case EQ:
	case NE:
	case LTU:
	case GEU:
	case LEU:
	case GTU:
	case GT:
	case LT:
	case LE:
	case GE:
	  return (C (0));

	default:
	  break;
      }
  if (n == 0)
    switch (outer_code)
      {
	case MINUS:
	case EQ:
	case NE:
	case LTU:
	case GEU:
	case LEU:
	case GTU:
	case GT:
	case LT:
	case LE:
	case GE:
	  return (C (0));

	default:
	  break;
      }
  if ((outer_code == PLUS && n == -1)
      || ((outer_code == AND || outer_code == XOR)
	 && (n == 255 || n == -256)))
    return (C (0));

  /* Most instructions can sign extend 8-bit constants.  Exceptions:
   * "movw $imm, reg", "testw $imm, dest" and "enterw $imm, $level".
   * We catch the "movw" case here.  We try to peephole "testw" away.
   * Some insn alternatives for "add", "andw", "orw" and "xorw" hack
   * zero extension of 8-bit constants.
   */
  if ((outer_code != SET && n >= -128 && n <= 127)
      || (outer_code == AND
	 && ((n & 0xff00) == 0xff00 || (n & 0x00ff) == 0x00ff))
      || ((outer_code == IOR || outer_code == XOR)
	 && ((n & 0xff00) == 0x0000 || (n & 0xff00) == 0x0000))
      || (outer_code == PLUS && (n & 0x00ff) == 0x0000))
    return (C (ia16_costs->byte_fetch));
  else
    return (C (ia16_costs->byte_fetch * GET_MODE_SIZE (mode)));
}

/* Return the cost of an address when not optimizing for an Intel i808x.
 */
static int
ia16_default_address_cost (rtx r1, rtx r2, rtx c)
{
  int total = 0;

  if (c)
    total = ia16_constant_cost (c, Pmode, MEM);

  /* If r1 == r2, a "movw" instruction is needed.  */
  if (rtx_equal_p (r1, r2))
     total += IA16_COST (move);

  return (total);
}

/* Calculate address cost for Intel i8086/i8088.  Note that %bp + %si or
 * %bx + %di costs one more cycle than %bp + %di or %bx + %si.
 * The %bp addressing mode is really 0 + %bp but we ignore that.
 */
static int
ia16_i808x_address_cost (rtx r1, rtx r2, rtx c)
{
  int cost = 0;

  if (r1)
    {
      if (r2)
        {
          cost = (C (7) + C (8)) / 2;

          /* If r1 == r2, a "movw" instruction is needed.  */
          if (rtx_equal_p (r1, r2))
            cost += IA16_COST (move);
        }
      else
        cost = C (5);

      if (c)
        cost += C (4) + ia16_constant_cost (c, Pmode, MEM);
    }
  else
    cost = C (6) + ia16_constant_cost (c, Pmode, MEM);;

  return (cost);
}

/* Size costs for IA-16 instructions.  Used when optimizing for size.
 * EA sizes are not included except for xlat.
 */
static struct processor_costs ia16_size_costs = {
  /* byte_fetch */	1,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (1), C (1) },
  /* imm_store */	{ C (2), C (2) },
  /* int_load */	{ C (2), C (2) },
  /* int_store */	{ C (2), C (2) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (2), C (2) },
  /* add_imm */		{ C (2), C (2) },
  /* inc_dec */		{ C (1), C (2) },
  /* lea */		C (2),
  /* cmp */		{ C (2), C (2), C (2) },
  /* cmp_imm */		{ C (2), C (2) },
  /* shift_1bit	*/	{ C (2), C (2) },
  /* shift_start */	{ C (2), C (2) },
  /* shift_bit */	C (0),
  /* s_mult_init */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* u_mult_init */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* mult_imm_init */	{ C (2), C (2) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* u_divide */	{ { C (2), C (2) }, { C (2), C (2) } },
  /* sign_extend */	{ C (1), C (1) },
  /* xlat */		C (1),
  /* call */		{ C (1), C (1), C (1) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel i8086 CPUs.  EA calculation time is not included.
 * Timings have been derived from the Intel i8088 timings.
 * TODO: .mult_bit is closer to 2 for signed multiplies.
 */
static struct processor_costs ia16_i8086_costs = {
  /* byte_fetch	*/	2,
  /* ea_calc */		ia16_i808x_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (10), C (10) },
  /* int_load */	{ C (8), C (8) },
  /* int_store */	{ C (9), C (9) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (9), C (16) },
  /* add_imm */		{ C (4), C (17) },
  /* inc_dec */		{ C (3), C (16) },
  /* lea */		C (2),
  /* cmp */		{ C (3), C (9), C (9) },
  /* cmp_imm */		{ C (4), C (10) },
  /* shift_1bit */	{ C (2), C (15) },
  /* shift_start */	{ C (8), C (20) },
  /* shift_bit */	C (4),
  /* s_mult_init */	{ { C (80), C (86) }, { C (128), C (134) } },
  /* u_mult_init */	{ { C (70), C (76) }, { C (118), C (124) } },
  /* mult_imm_init */	{ C (132), C (138) },
  /* mult_bit */	C (1),
  /* s_divide */	{ { C (106), C (112) }, { C (174), C (180) } },
  /* u_divide */	{ { C (85), C (91) }, { C (153), C (159) } },
  /* sign_extend */	{ C (2), C (5) },
  /* xlat */		C (11),
  /* call */		{ C (19), C (16), C (21) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 8088 CPUs.  EA calculation time is not included.  */
/* TODO: .mult_bit is closer to 2 for signed multiplies.  */
static struct processor_costs ia16_i8088_costs = {
  /* byte_fetch */	4,
  /* ea_calc */		ia16_i808x_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (10), C (14) },
  /* int_load */	{ C (8), C (12) },
  /* int_store */	{ C (9), C (13) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (13), C (24) },
  /* add_imm */		{ C (4), C (25) },
  /* inc_dec */		{ C (3), C (24) },
  /* lea */		C (2),
  /* cmp */		{ C (3), C (13), C (13) },
  /* cmp_imm */		{ C (4), C (14) },
  /* shift_1bit */	{ C (2), C (23) },
  /* shift_start */	{ C (8), C (28) },
  /* shift_bit */	C (4),
  /* s_mult_init */	{ { C (80), C (86) }, { C (128), C (138) } },
  /* u_mult_init */	{ { C (70), C (76) }, { C (118), C (128) } },
  /* mult_imm_init */	{ C (132), C (142) },
  /* mult_bit */	C (1),
  /* s_divide */	{ { C (106), C (112) }, { C (174), C (184) } },
  /* u_divide */	{ { C (85), C (91) }, { C (153), C (163) } },
  /* sign_extend */	{ C (2), C (5) },
  /* xlat */		C (11),
  /* call */		{ C (23), C (20), C (29) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 80186 CPUs.  EA calculation time (4 cycles for all modes)
 * is included wherever appropriate.  */
/* TODO: Intel 80188 timings.  */
static struct processor_costs ia16_i80186_costs = {
  /* byte_fetch	*/	2,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (3), C (4) },
  /* imm_store */	{ C (12), C (13) },
  /* int_load */	{ C (9), C (9) },
  /* int_store */	{ C (12), C (12) },
  /* move_ratio */	C (4),
  /* add */		{ C (3), C (10), C (15) },
  /* add_imm */		{ C (4), C (16) },
  /* inc_dec */		{ C (3), C (15) },
  /* lea */		C (6),
  /* cmp */		{ C (3), C (10), C (10) },
  /* cmp_imm */		{ C (3), C (10) },
  /* shift_1bit */	{ C (2), C (15) },
  /* shift_start */	{ C (5), C (17) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (26), C (32) }, { C (35), C (41) } },
  /* u_mult_init */	{ { C (27), C (33) }, { C (36), C (42) } },
  /* mult_imm_init */	{ C (23), C (30) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (48), C (54) }, { C (57), C (63) } },
  /* u_divide */	{ { C (29), C (35) }, { C (38), C (44) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (11),
  /* call */		{ C (15), C (13), C (19) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for Intel 80286 CPUs.  EA calculation time (? cycles for all modes)
 * is included wherever appropriate.  */
/* TODO: Check timings for signed vs. unsigned multiply.  */
static struct processor_costs ia16_i80286_costs = {
  /* byte_fetch */	1, /* guess */
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (2), C (2) },
  /* imm_store */	{ C (3), C (3) },
  /* int_load */	{ C (5), C (5) },
  /* int_store */	{ C (3), C (3) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (7), C (7) },
  /* add_imm */		{ C (3), C (7) },
  /* inc_dec */		{ C (2), C (7) },
  /* lea */		C (3),
  /* cmp */		{ C (3), C (6), C (7) },
  /* cmp_imm */		{ C (3), C (6) },
  /* shift_1bit */	{ C (2), C (7) },
  /* shift_start */	{ C (5), C (8) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (13), C (16) }, { C (21), C (24) } },
  /* u_mult_init */	{ { C (13), C (16) }, { C (21), C (24) } },
  /* mult_imm_init */	{ C (21), C (24) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (17), C (20) }, { C (25), C (28) } },
  /* u_divide */	{ { C (14), C (17) }, { C (22), C (25) } },
  /* sign_extend */	{ C (2), C (2) },
  /* xlat */		C (5),
  /* call */		{ C (7), C (7), C (11) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for NEC V30 CPUs.  EA calculation time (2 cycles for all modes)
 * is included wherever appropriate.  Timings are derived from the V20 ones.
 */
static struct processor_costs ia16_nec_v30_costs = {
  /* byte_fetch */	2,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (11), C (11) },
  /* int_load */	{ C (11), C (11) },
  /* int_store */	{ C (9), C (9) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (11), C (16) },
  /* add_imm */		{ C (4), C (18) },
  /* inc_dec */		{ C (2), C (16) },
  /* lea */		C (4),
  /* cmp */		{ C (2), C (11), C (11) },
  /* cmp_imm */		{ C (4), C (13) },
  /* shift_1bit */	{ C (2), C (16) },
  /* shift_start */	{ C (7), C (19) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (33), C (39) }, { C (47), C (53) } },
  /* u_mult_init */	{ { C (21), C (30) }, { C (27), C (32) } },
  /* mult_imm_init */	{ C (31), C (34) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (29), C (35) }, { C (43), C (49) } },
  /* u_divide */	{ { C (19), C (25) }, { C (25), C (31) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (9),
  /* call */		{ C (16), C (14), C (23) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

/* Costs for NEC V20 CPUs.  EA calculation time (2 cycles for all modes)
 * is included wherever appropriate.  */
static struct processor_costs ia16_nec_v20_costs = {
  /* byte_fetch */	4,
  /* ea_calc */		ia16_default_address_cost,
  /* move */		C (2),
  /* imm_load */	{ C (4), C (4) },
  /* imm_store */	{ C (11), C (15) },
  /* int_load */	{ C (11), C (15) },
  /* int_store */	{ C (9), C (13) },
  /* move_ratio */	C (4),
  /* add */		{ C (2), C (15), C (24) },
  /* add_imm */		{ C (4), C (26) },
  /* inc_dec */		{ C (2), C (24) },
  /* lea */		C (4),
  /* cmp */		{ C (2), C (15), C (15) },
  /* cmp_imm */		{ C (4), C (17) },
  /* shift_1bit */	{ C (2), C (24) },
  /* shift_start */	{ C (7), C (27) },
  /* shift_bit */	C (1),
  /* s_mult_init */	{ { C (33), C (39) }, { C (47), C (57) } },
  /* u_mult_init */	{ { C (21), C (30) }, { C (27), C (36) } },
  /* mult_imm_init */	{ C (31), C (38) },
  /* mult_bit */	C (0),
  /* s_divide */	{ { C (29), C (35) }, { C (43), C (53) } },
  /* u_divide */	{ { C (19), C (25) }, { C (25), C (35) } },
  /* sign_extend */	{ C (2), C (4) },
  /* xlat */		C (9),
  /* call */		{ C (20), C (18), C (31) },
  0, { 0, 0, 0 }, { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0,
};

#undef C

const struct processor_costs *ia16_costs = &ia16_i8086_costs;

#define I_REG	0
#define I_MEM	1
#define O_REGREG 0
#define O_MEMREG 1
#define O_REGMEM 2
#define M_QI	0
#define M_HI	1

#define I_RTX(x)	(MEM_P (x) ? I_MEM : I_REG)
#define M_MOD(x)	(QImode == (x) ? M_QI : M_HI)
#define M_RTX(x)	(QImode == GET_MODE (x) ? M_QI : M_HI)

#undef  TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST ia16_address_cost

static int
ia16_address_cost (rtx address)
{
  rtx r1, r2, c;

  /* Addresses in stack pushes/pops are cheap.  */
  if (PRE_MODIFY == GET_CODE (address)
      || POST_MODIFY == GET_CODE (address)
      || PRE_DEC == GET_CODE (address)
      || POST_INC == GET_CODE (address))
    return (0);

  /* Check for a (call ...) address.  This is a hack, but without knowing
   * the mode, this is the best we can do.  At least it won't crash.  */
  if (MEM_P (address))
    address = XEXP (address, 0);

  /* Parse the address.  */
  ia16_parse_address (address, &r1, &r2, &c);

  return (ia16_costs->ea_calc (r1, r2, c));
}

/* Return true if X is a valid memory operand for xlat and return the cost
 * in TOTAL.  Don't touch TOTAL otherwise.
 * This is complicated because combine uses many different ways of expressing
 * xlat.  We support:
 * (mem:QI (plus:HI (zero_extend:HI (mem_subreg_reg:QI op1)) (base:HI))
 * (mem:QI (plus:HI (zero_extract:HI (mem_subreg_reg:HI op1) (8) (8)) (base:HI))
 * (mem:QI (plus:HI (and:HI (stuff:HI op1) (255)) (base:HI))
 * (mem:QI (plus:HI (lshiftrt:HI (stuff) (const_int fubar)) (base:HI)))
 * and strip subregs as necessary.
 */
static bool
ia16_xlat_cost (rtx x, int *total)
{
      if (GET_MODE (x) != QImode)
        return (false);
      if (GET_CODE (XEXP (x, 0)) == PLUS)
	{
	  rtx plus, zext, op1, base;

	  plus = XEXP (x, 0);
	  zext = XEXP (plus, 0);
	  base = XEXP (plus, 1);
	  if (GET_CODE (zext) == SUBREG)
	    zext = SUBREG_REG (zext);
	  if (((GET_CODE (zext) != ZERO_EXTEND && GET_CODE (zext) != ZERO_EXTRACT
	        && (GET_CODE (zext) != AND || !CONST_INT_P (XEXP (zext, 1))
	            || INTVAL (XEXP (zext, 1)) != 255))
	        && (GET_CODE (zext) != LSHIFTRT || !CONST_INT_P (XEXP (zext, 1))))
	      || (!general_operand (base, Pmode)))
	    return (false);
	  
	  op1 = XEXP (zext, 0);
/*	  op2 = XEXP (zext, 1); */
	  if (GET_CODE (op1) == SUBREG)
	    op1 = SUBREG_REG (op1);
	  if (!REG_P (op1) && !MEM_P (op1)
	      && (GET_CODE (op1) != LSHIFTRT /*|| !CONST_INT_P (op2)*/))
	    return (false);
	  *total = IA16_COST (xlat);

	  /* We may need to load the extendee into a register.  */
	  if (!REG_P (op1))
	    *total += IA16_COST (int_load[M_QI]);

	  /* We may need to load the base into a register.  */
	  if (CONSTANT_P (base))
	    *total += IA16_COST (imm_load[M_HI]);
	  else if (MEM_P (base))
	    *total += IA16_COST (int_load[M_HI])
		      + ia16_address_cost (XEXP (base, 0));
	  return (true);	
	}
      return (false);
}

#undef  TARGET_RTX_COSTS
#define TARGET_RTX_COSTS	ia16_rtx_costs

/* Compute a (partial) cost for rtx X.  Return true if the complete
   cost has been computed, and false if subexpressions should be
   scanned.  In either case, *TOTAL contains the cost result.
   FIXME: Most costs are too low for modes larger than HImode.
   FIXME: When called from combine, we only ever see the source of SET
   expressions. We don't want to return a higher cost for
	(set (mem) (and (mem) (reg1))
   than we do for
	(set (reg2) (mem))
	(set (reg2) (and (reg2) (reg1))
	(set (mem) (reg2))
   so don't count the MEM twice if we get only the source expression.
*/

static bool
ia16_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  switch (code)
    {
    case SET:
      {
	rtx op0 = XEXP (x, 0);
	rtx op1 = XEXP (x, 1);

	if (memory_operand (op0, GET_MODE (op0)))
	  if (register_operand (op1, GET_MODE (op1)))
	    *total = IA16_COST (int_store[M_RTX (op0)]);
	  else if (immediate_operand (op1, GET_MODE (op1)))
	    *total = IA16_COST (imm_store[M_RTX (op0)]);
	  else
#if 0
	    /* Arithmetic or logic instruction which writes to memory.
	     * Subtract the mem,reg->reg cost which will incorrectly be added
	     * in the MEM case below.  */
	    *total = IA16_COST (add[O_REGMEM]) - IA16_COST (add[O_MEMREG]);
#else
	    do {} while (0);
#endif
	else if (register_operand (op0, GET_MODE (op1)))
	  if (register_operand (op1, GET_MODE (op1)))
	    *total = IA16_COST (move);
	  else if (memory_operand (op1, GET_MODE (op1)))
	    *total = IA16_COST (int_load[M_RTX (op1)]);
	  else if (immediate_operand (op1, GET_MODE (op1)))
	    *total = IA16_COST (imm_load[M_RTX (op0)]);

	  /* Try to get constructs involving "xlat" right.  */
	  else if (MEM_P (op1) && ia16_xlat_cost (op1, total))
	    return (true);
	return (false);
      }

    /* It is not easy to give an accurate cost of a memory access.  "movw" is
     * handled above.  Return a reasonable estimate for arithmetic or logic
     * instructions.  */
    case MEM:
      if ((outer_code == ZERO_EXTEND || outer_code == SET)
	&& ia16_xlat_cost (x, total))
	return (true);
      *total = ia16_address_cost (XEXP (x, 0));
      /* This is never called recursively from the SET case.  */
      if (outer_code == SET)
	*total += IA16_COST (int_load[M_MOD (mode)]);
#if 0 /* This is handled individually for each code.  */
      else
	*total += IA16_COST (add[O_MEMREG]) - IA16_COST (add[O_REGREG]);
#endif
      return (true);

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
    case CONST_INT:
    case CONST_VECTOR:
      *total = ia16_constant_cost (x, mode, outer_code);
      /* Take counter measures against prepare_cmp_insn().  */
      if (outer_code == COMPARE && *total > COSTS_N_INSNS (1))
	*total = COSTS_N_INSNS (1);
      /* This is never called recursively from the SET case.  */
      if (outer_code == SET)
	*total += IA16_COST (imm_load[M_HI]);
      return true;

    case CONST_DOUBLE:
      *total = ia16_costs->byte_fetch * COSTS_N_INSNS (4);
      return true;

    /* This is usually "xor reg,reg" or "mov $0, dest".  */
    case ZERO_EXTEND:
      if (outer_code == IOR || outer_code == XOR)
	*total = 0;
      else
	*total = IA16_COST (add[I_RTX (XEXP (x, 0))]);

      /* Avoid the extra high cost of a subreg.  */
      if (GET_CODE (XEXP (x, 0)) == SUBREG)
	{
	  *total += rtx_cost (SUBREG_REG (XEXP (x, 0)), code);
	  return true;
	}
      return false;

    case SIGN_EXTEND:
      *total = IA16_COST (sign_extend[M_MOD (GET_MODE (XEXP (x, 0)))]);
      return false;

    /* TODO: Constant shift/rotate when !TARGET_SHIFT_IMM.  */
    case ASHIFTRT:
      if (outer_code != COMPARE && CONST_INT_P (XEXP (x, 1)))
	{
	  /* This is implemented using sign extension.  */
	  if (mode == QImode && INTVAL (XEXP (x, 1)) >= 7)
	    if (outer_code == SIGN_EXTEND)
	      *total = 0;
	    else
	      *total = IA16_COST (sign_extend[M_QI]);
	  else if (mode == HImode && INTVAL (XEXP (x, 1)) >= 15)
	    *total = IA16_COST (sign_extend[M_HI]);
          *total += rtx_cost (XEXP (x, 0), code);
          return true;
	}
      /* fall through */
    case ASHIFT:
      /* Sometimes we get the ASHIFT and the constant for free.  */
      if (code == ASHIFT && mode == HImode
	  && (outer_code == PLUS || outer_code == MINUS
	     || outer_code == IOR || outer_code == XOR)
          && CONST_INT_P (XEXP (x, 1))
          && INTVAL (XEXP (x, 1)) == 8)
	{
	  *total = rtx_cost (XEXP (x, 0), code);
	  return true;
	}
    case LSHIFTRT:
      /* Similiarly with LSHIFTRT.  */
      if (code == LSHIFTRT && mode == HImode
	  && (outer_code == IOR || outer_code == XOR)
	  && CONST_INT_P (XEXP (x, 1))
	  && INTVAL (XEXP (x, 1)) == 8)
	{
	  *total = rtx_cost (XEXP (x, 0), code);
	  return true;
        }
    case ROTATE:
    case ROTATERT:
      if (CONST_INT_P (XEXP (x, 1)))
	if (mode == HImode && outer_code != COMPARE
	 && INTVAL (XEXP (x, 1)) == 8)
	  {
	    /* This is implemented using movb or xchgb.  */
	    *total = IA16_COST (add[O_REGREG])
	           + rtx_cost (XEXP (x, 0), code);
	    return true;
	  }
	else if (INTVAL (XEXP (x, 1)) == 1)
	  {
	    *total = IA16_COST (shift_1bit[I_REG])
	           + rtx_cost (XEXP (x, 0), code);
	  }
	else
	  *total = MAX (ia16_costs->shift_start[I_REG]
		        + ia16_costs->shift_bit * INTVAL (XEXP (x, 1)),
			ia16_size_costs.shift_start[I_REG]
			* ia16_costs->byte_fetch);
      else
	*total = MAX (ia16_costs->shift_start[I_REG]
		      + ia16_costs->shift_bit
			* GET_MODE_BITSIZE (GET_MODE (XEXP (x, 1))) / 2,
		      ia16_size_costs.shift_start[I_REG]
			* ia16_costs->byte_fetch);

      /* (subreg:QI (ashift:HI (zero_extend:HI (mem:QI ...))) 0) */
      if (outer_code == SUBREG
	  && (ZERO_EXTEND == GET_CODE (XEXP (x, 0))
	      || SIGN_EXTEND == GET_CODE (XEXP (x,0))))
	{
	  *total += rtx_cost (XEXP (XEXP (x, 0), 0), code);
	  return true;
	}
      else
	return false;

    case MULT:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fmul);
	  return false;
	}
      else
	{
	  rtx op0 = XEXP (x, 0);
	  rtx op1 = XEXP (x, 1);
	  int nbits;

	  /* (mult:HI (...) (const_int 257) is cheap.  */
	  if (CONST_INT_P (op1) && INTVAL (op1) == 257 && mode == HImode)
	    {
	      *total = IA16_COST (add[O_REGREG]) + rtx_cost (op0, code);
	      return true;
            }
	  if (CONST_INT_P (op1) && ia16_costs->mult_bit != 0)
	    {
	      unsigned HOST_WIDE_INT value = INTVAL (XEXP (x, 1));
	      for (nbits = 0; value != 0; value &= value - 1)
	        nbits++;
	    }
	  else
	    /* This is arbitrary.  */
	    nbits = 7;

	  /* Compute costs correctly for widening multiplication.  */
	  if ((GET_CODE (op0) == SIGN_EXTEND || GET_CODE (op1) == ZERO_EXTEND)
	      && GET_MODE_SIZE (GET_MODE (XEXP (op0, 0))) * 2
	         == GET_MODE_SIZE (mode))
	    {
	      int is_mulwiden = 0;
	      enum machine_mode inner_mode = GET_MODE (op0);

	      if (GET_CODE (op0) == GET_CODE (op1))
		is_mulwiden = 1, op1 = XEXP (op1, 0);
	      else if (CONST_INT_P (op1))
		{
		  if (GET_CODE (op0) == SIGN_EXTEND)
		    is_mulwiden = trunc_int_for_mode (INTVAL (op1), inner_mode)
			          == INTVAL (op1);
		  else
		    is_mulwiden = !(INTVAL (op1) & ~GET_MODE_MASK (inner_mode));
	        }

	      if (is_mulwiden)
	        op0 = XEXP (op0, 0), mode = GET_MODE (op0);
	    }

	  /* FIXME: Check for signed vs. unsigned.  */
  	  *total = MAX (ia16_costs->s_mult_init[M_MOD (mode)][I_RTX (op1)]
			+ nbits * ia16_costs->mult_bit,
			ia16_size_costs.s_mult_init[M_MOD (mode)][I_RTX (op1)]
			* ia16_costs->byte_fetch)
	           + rtx_cost (op0, outer_code) + rtx_cost (op1, outer_code);

          return true;
	}

    /* FIXME: Docs are unclear about UDIV/UMOD on floating point modes.  */
    case UDIV:
    case UMOD:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fdiv);
      else
	*total = IA16_COST (u_divide[M_MOD (mode)][I_RTX (XEXP (x, 0))]);
      return false;

    case DIV:
    case MOD:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fdiv);
      else
	*total = IA16_COST (s_divide[M_MOD (mode)][I_RTX (XEXP (x, 0))]);
      return false;

    /* FIXME: This might be too pessimistic when rtx_equal_p (op0, op1).  */
    case PLUS:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fadd);
	  return false;
	}
      /* PLUS with -1 or 1 is cheaper.  */
      if (XEXP (x, 1) == const1_rtx || XEXP (x, 1) == constm1_rtx)
	{
	  *total = IA16_COST (inc_dec[I_RTX (x)]);
	  return (false);
	}
      /* "shl $1, dest" is preferred over "add dest, dest".  */
      if (GET_MODE_SIZE (mode) <= 2
	  && rtx_equal_p (XEXP (x, 0), XEXP (x, 1)))
	{
	  *total = 1 + IA16_COST (add[I_RTX (x)])
		   * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
	  return (false);
	}
      /* Compute cost of "leaw" instruction.  */
      if (mode == HImode)
	{
	  if (GET_CODE (XEXP (x, 0)) == PLUS
	    && CONSTANT_P (XEXP (x,1)))
	    {
	      rtx op0 = XEXP (XEXP (x, 0), 0);
	      rtx op1 = XEXP (XEXP (x, 0), 1);
	      rtx cst = XEXP (x, 1);

	      *total = IA16_COST (lea) + ia16_costs->ea_calc (op0, op1, cst);
		     + rtx_cost (op0, outer_code);
		     + rtx_cost (op1, outer_code);
	      return true;
	    }
	}
      /* Detect *subm3_carru.  */
      if (outer_code == MINUS
	  && carry_flag_operator (XEXP (x, 0), mode))
	{
	  *total = rtx_cost (XEXP (x, 1), outer_code);
	  return false;
	}
      /* FALLTHRU */

    case MINUS:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fadd);
	  return false;
	}
      /* Detect *addm3_carry.  */
      if (outer_code == PLUS
	  && carry_flag_operator (XEXP (x, 1), mode))
	{
	  *total = rtx_cost (XEXP (x, 0), outer_code);
	  return false;
	}
      /* FALLTHRU */

    case AND:
      /* Sometimes we get both the AND and the constant for free.  */
      if (code == AND && mode == HImode
	  && (outer_code == PLUS || outer_code == MINUS
	     || outer_code == IOR || outer_code == XOR)
          && CONST_INT_P (XEXP (x, 1))
          && INTVAL (XEXP (x, 1)) == -256)
	{
	  *total = rtx_cost (XEXP (x, 0), code);
	  return true;
        }
    case IOR:
    case XOR:
      if (CONSTANT_P (XEXP (x, 1)))
	*total = IA16_COST (add_imm[I_RTX (XEXP (x, 0))])
	       * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      else
	*total = IA16_COST (add[I_RTX (XEXP (x, 0))])
	       * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      return false;

    case NEG:
      if (FLOAT_MODE_P (mode))
	{
	  *total = IA16_COST (fchs);
	  return false;
	}
      if (carry_not_flag_operator (XEXP (x, 0), mode))
	{
	  *total = 0;
	  return false;
        }
      /* FALLTHRU */

    case NOT:
      *total = IA16_COST (add[I_RTX (XEXP (x, 0))])
	     * (mode == QImode ? 1 : GET_MODE_SIZE (mode) / UNITS_PER_WORD);
      return false;

    case COMPARE:
      if (GET_CODE (XEXP (x, 0)) == ZERO_EXTRACT
	  && XEXP (XEXP (x, 0), 1) == const1_rtx
	  && CONST_INT_P (XEXP (XEXP (x, 0), 2))
	  && XEXP (x, 1) == const0_rtx)
	{
	  /* This kind of construct is implemented using test[bw].
	     Treat it as if we had an AND.  */
	  *total = (IA16_COST (add_imm[I_RTX (x)])
		    + rtx_cost (XEXP (XEXP (x, 0), 0), outer_code)
		    + rtx_cost (const1_rtx, outer_code));
	  return true;
	}
      /* COMPARE is free in e.g. (compare (plus ...) (const_int 0)).  */
      if ((GET_RTX_CLASS (GET_CODE (XEXP (x, 0))) == RTX_COMM_ARITH
	   || GET_RTX_CLASS (GET_CODE (XEXP (x, 0))) == RTX_BIN_ARITH)
	  && XEXP (x, 1) == const0_rtx)
	{
	  *total = rtx_cost (XEXP (x, 0), outer_code);
	  return true;
	}
      /* COMPARE is free in overflow checks.  */
      if (GET_CODE (XEXP (x, 0)) == MINUS
	  && rtx_equal_p (XEXP (x, 1), XEXP (XEXP (x, 0), 0)))
	{
	  *total = rtx_cost (XEXP (x, 0), outer_code);
	  return true;
	}
      if (CONSTANT_P (XEXP (x, 1)))
	{
	  *total = IA16_COST (cmp_imm[I_RTX (XEXP (x, 0))]);
	  if (XEXP (x, 1) == const0_rtx
	      && (REG_P (XEXP (x, 0)) || GET_CODE (XEXP (x, 0)) == SUBREG))
	    return true;
	}
      else
	*total = IA16_COST (cmp[I_RTX (XEXP (x, 0))]);
      return false;

    case ZERO_EXTRACT:
      if (outer_code == COMPARE)
	{
	  /* See above under COMPARE.  */
	  *total = (IA16_COST (add[I_RTX (x)])
		    + rtx_cost (XEXP (x, 0), outer_code)
		    + rtx_cost (const1_rtx, outer_code));
	  return true;
	}
      if (REG_P (x))
	*total = IA16_COST (move);
      else
	*total = IA16_COST (int_load[M_QI]);
      return false;

    case FLOAT_EXTEND:
	*total = 0;
      return false;

    case ABS:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fabs);
      else
	/* This is expanded to cwtd, xorw and subw.  */
	*total = IA16_COST (sign_extend[M_MOD (mode)])
	       + IA16_COST (add[I_REG]) * 2;
      return false;

    case SQRT:
      if (FLOAT_MODE_P (mode))
	*total = IA16_COST (fsqrt);
      return false;

    case TRUNCATE:
      *total = 0;
      return true;

    case CALL:
      if (CONSTANT_P (XEXP (x, 0)))
	*total = IA16_COST (call[0]);
      else if (REG_P (XEXP (x, 0)))
	*total = IA16_COST (call[1]);
      else
	*total = IA16_COST (call[2]);
      return false;

    case LTU:
    case GEU:
    case LEU:
    case GTU:
    case GT:
    case LT:
    case LE:
    case GE:
    case NE:
    case EQ:
      if (outer_code != SET && outer_code != NEG)
	*total = 0;
      else if (carry_not_flag_operator (x, mode))
	*total = IA16_COST (add[O_REGREG])
	       + outer_code == SET ? COSTS_N_INSNS (ia16_costs->byte_fetch) : 0
	       + outer_code == NEG ? IA16_COST (add[I_REG]) : 0;
      else if (outer_code != SET)
	*total = 0;
      else if (carry_flag_operator (x, mode))
	*total = IA16_COST (add[O_REGREG]);
      else if (code == LT)
	*total = COSTS_N_INSNS (ia16_costs->byte_fetch)
	       + IA16_COST (sign_extend[M_MOD (HImode)]);
      else if (code == EQ)
	*total = COSTS_N_INSNS (ia16_costs->byte_fetch)
	       + IA16_COST (shift_1bit[I_REG])
	       + IA16_COST (sign_extend[M_MOD (HImode)]);
      return true;

    case UNSPEC:
      /* Instructions like cmc and lahf.  Assume they are cheap.  */
      *total = COSTS_N_INSNS (ia16_costs->byte_fetch);
      return false;

    /* cse_insn() likes to compute costs of EXPR_LIST rtxes.  */
    case EXPR_LIST:

    /* TODO */
    /* Check outer_code here (e.g. SET, MINUS).  */
    case IF_THEN_ELSE:
    case PC:
    case CLZ:
    case CTZ:
    case FFS:
    case PARITY:
    case POPCOUNT:
    case ASM_INPUT:
    case ASM_OPERANDS:
    case CONCAT:
      return false;

    default:
      fprintf (stderr, "Unknown cost for rtx with code %s and "
               "outer code %s:\n", GET_RTX_NAME (code),
	       GET_RTX_NAME (outer_code));
      debug_rtx (x);
      return false;
    }
}

/* Continued: Run-time Target Specification */
#undef  TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION	ia16_handle_option
static bool
ia16_handle_option (size_t code, const char *arg ATTRIBUTE_UNUSED, int value ATTRIBUTE_UNUSED)
{
  if (optimize_size)
    {
      ia16_costs = &ia16_size_costs;
      return (true);
    }

  switch (code)
    {
      case OPT_march_i8086:
        target_tune &= ~8;
        /* fall through. */

      case OPT_mtune_i8086:
	ia16_costs = &ia16_i8086_costs;
	return (true);

      case OPT_march_i8088:
        target_tune |= 8;
        /* fall through. */

      case OPT_mtune_i8088:
	ia16_costs = &ia16_i8088_costs;
	return (true);

      case OPT_march_i80186:
        target_tune &= ~8;
        /* fall through. */

      case OPT_mtune_i80186:
	ia16_costs = &ia16_i80186_costs;
	return (true);

      case OPT_march_i80188:
        target_tune |= 8;
        /* fall through. */

      case OPT_mtune_i80188:
	ia16_costs = &ia16_i80186_costs;
	return (true);

      case OPT_march_i80286:
        target_tune &= ~8;
        /* fall through. */

      case OPT_mtune_i80286:
	ia16_costs = &ia16_i80286_costs;
	return (true);

      case OPT_march_v20:
        target_tune |= 8;
        /* fall through. */

      case OPT_mtune_v20:
	ia16_costs = &ia16_nec_v20_costs;
	return (true);

      case OPT_march_v30:
        target_tune &= ~8;
        /* fall through. */

      case OPT_mtune_v30:
	ia16_costs = &ia16_nec_v30_costs;
	return (true);
    }
  return (true);
}

/* The Overall Framework of an Assembler File */
#undef  TARGET_ASM_FILE_START
#undef  TARGET_ASM_FILE_START_APP_OFF
#define TARGET_ASM_FILE_START			ia16_asm_file_start
#define TARGET_ASM_FILE_START_APP_OFF		1

/* GAS doesn't support .file.  */
#undef  TARGET_ASM_FILE_START_FILE_DIRECTIVE
#define TARGET_ASM_FILE_START_FILE_DIRECTIVE	false

/* Output assembler directives to set up the GNU assembler for the CPU type,
 * code model and assembler syntax that we use.  */
static void
ia16_asm_file_start (void)
{
	const char *arch, *code;

	if (TARGET_FSTSW_AX)
		arch = "i286";
	else if (TARGET_PUSH_IMM)
		arch = "i186";
	else
		arch = "i8086";
	code = "16";

	fprintf (asm_out_file,	"\t.arch %s,jumps\n"
				"\t.code%s\n"
				"\t.att_syntax prefix\n", arch, code);
	default_file_start ();
}

/* Output of Data */
#undef  TARGET_ASM_BYTE_OP
#undef  TARGET_ASM_ALIGNED_HI_OP
#undef  TARGET_ASM_ALIGNED_SI_OP
#undef  TARGET_ASM_ALIGNED_DI_OP
#undef  TARGET_ASM_ALIGNED_TI_OP
#undef  TARGET_ASM_UNALIGNED_HI_OP
#undef  TARGET_ASM_UNALIGNED_SI_OP
#undef  TARGET_ASM_UNALIGNED_DI_OP
#undef  TARGET_ASM_UNALIGNED_TI_OP
#define TARGET_ASM_BYTE_OP		"\t.byte\t"
#define TARGET_ASM_ALIGNED_HI_OP	"\t.hword\t"
#define TARGET_ASM_ALIGNED_SI_OP	"\t.long\t"
#define TARGET_ASM_ALIGNED_DI_OP	"\t.quad\t"
#define TARGET_ASM_ALIGNED_TI_OP	"\t.octa\t"
#define TARGET_ASM_UNALIGNED_HI_OP	"\t.hword\t"
#define TARGET_ASM_UNALIGNED_SI_OP	"\t.long\t"
#define TARGET_ASM_UNALIGNED_DI_OP	"\t.quad\t"
#define TARGET_ASM_UNALIGNED_TI_OP	"\t.octa\t"

static const char *reg_QInames[SI_REG] = {
	"cl", "ch", "al", "ah", "dl", "dh", "bl", "bh"
};

static const char *reg_HInames[SP_REG+1] = {
	"cx", 0, "ax", 0, "dx", 0, "bx", 0, "si", "di", "bp", "es", "sp"
};

/* E is known not to be null when this is called.  These non-standard codes are
   supported:
   'L': Print the name of the lower 8-bits of E. 
   'H': Print the name of the upper 8-bits of E.
   'X': Print the name of E as a 16-bit operand.
   'R': Don't print any register prefix before E.
*/
void
ia16_print_operand (FILE *file, rtx e, int code)
{
  enum machine_mode mode;
  unsigned int regno;
  rtx x;

  /* simplify_gen_subreg() is used to allow the 'X', 'H' and 'L' output
   * modifiers to be used for REG, MEM and constant expressions.  The cases
   * which simplify_subreg() refuses to handle are handled by alter_subreg().
   */
  if (code == 'X')
    x = simplify_gen_subreg (HImode, e, QImode, 0);
  else if (code == 'L')
    if (GET_MODE (e) == VOIDmode)
      x = simplify_gen_subreg (QImode, e, HImode, 0);
    else
      x = simplify_gen_subreg (QImode, e, GET_MODE (e), 0);
  else if (code == 'H')
    x = simplify_gen_subreg (QImode, e, HImode, 1);
  else
    x = e;
  if (GET_CODE (x) == SUBREG)
    x = alter_subreg (&x);
  mode = GET_MODE (x);

  switch (GET_CODE (x))
    {
      case REG:
      regno = REGNO (x);
      if (code != 'R')
	fputs (REGISTER_PREFIX, file);
      if (GET_MODE_SIZE (mode) >= 2
       && (regno >= SI_REG || (regno & 1) == 0))
	fputs (reg_HInames[regno], file);
      else if (GET_MODE_SIZE (mode) == 1 && regno < SI_REG)
	fputs (reg_QInames[regno], file);
      else
	output_operand_lossage ("Invalid register %s (nr. %u) in %smode.",
	                        reg_names [REGNO (x)], REGNO (x),
	                        GET_MODE_NAME (GET_MODE (x)));
      break;

      case CONST_VECTOR:
      fprintf (file, "%s" HOST_WIDE_INT_PRINT_DEC, IMMEDIATE_PREFIX,
	       INTVAL (XVECEXP (x, 0, 0)) + 256 * INTVAL (XVECEXP (x, 0, 1)));
      break;

      /* TODO: handle floating point constants here.  */

      case CONST_INT:
      case CONST:
      case SYMBOL_REF:
      case LABEL_REF:
      fputs (IMMEDIATE_PREFIX, file);
      /* fall through */

      case CODE_LABEL:
      output_addr_const (file, x);
      break;

      case MEM:
      ia16_print_operand_address (file, XEXP (x, 0));
      break;

      default:
      debug_rtx (e);
      output_operand_lossage ("Invalid or unsupported operand %s (code"
			      " %c).", rtx_name [GET_CODE (e)], code);
	break;
  }
}

#define INDEX_REG_P(x) \
	TEST_HARD_REG_BIT (reg_class_contents[INDEX_REGS], REGNO (x))
#define BASE_REG_REG_P(x) \
	TEST_HARD_REG_BIT (reg_class_contents[BASE_W_INDEX_REGS], REGNO (x))

/* Strictly check an address X and optionally split into its components.
 * If there is only one register, it will be the base register.
 * This is a helper function for ia16_print_operand_address ().
 */
static bool 
ia16_parse_address_strict (rtx x, rtx *p_rb, rtx *p_ri, rtx *p_c)
{
	rtx tmp;
	rtx rb, ri, c;
	enum machine_mode mode;

	if (!ia16_parse_address (x, &rb, &ri, &c))
		return (0 == 1);
	mode = GET_MODE (x);

	/* Swap the registers if necessary.  */
	if (rb && ri && INDEX_REG_P (rb) && BASE_REG_REG_P (ri))
	  {
		tmp = rb; rb = ri; ri = tmp;
	  }
	/* Check register classes for base + index.  */
	if (rb && ri
	    && (!INDEX_REG_P (ri)
		|| !BASE_REG_REG_P (rb)))
		return (0 == 1);

	/* Check register class for base.  */
	if (rb && !ri && !REG_MODE_OK_FOR_BASE_P (rb, mode))
		return (0 == 1);

	if (p_rb)
		*p_rb = rb;
	if (p_ri)
		*p_ri = ri;
	if (p_c)
		*p_c = c;

	return (0 == 0);
}

/* Possible asm operands and their address expressions:
 * c:		(const_int c)
 * (rb):	(reg rb)
 * c(rb):	(plus (reg rb) (const_int c))
 * (rb, ri):	(plus (reg rb) (reg ri))
 * c(rb, ri):	(plus (plus (reg rb) (reg ri)) (const_int c))
 */
void
ia16_print_operand_address (FILE *file, rtx e)
{
	rtx rb, ri, c;

	if (!ia16_parse_address_strict (e, &rb, &ri, &c)) {
        	debug_rtx (e);
        	output_operand_lossage ("Invalid IA16 address expression.");
        	return;
	}
	if (c)
		output_addr_const (file, c);
	if (rb)
		fprintf (file, "(%s%s",
		         REGISTER_PREFIX, reg_HInames[REGNO (rb)]);
	if (ri)
		fprintf (file, ",%s%s",
			 REGISTER_PREFIX, reg_HInames[REGNO (ri)]);
	if (rb)
		putc (')', file);
}

/* Helper function for gen_prologue().  Generates RTL to push REGNO, which
 * must be a 16-bit register, onto the stack.  */
rtx
ia16_push_reg (unsigned int regno)
{
  return gen__pushhi1_prologue (gen_rtx_REG (HImode, regno));
}

/* Helper function for gen_epilogue().  Generates RTL to pop REGNO, which
 * must be a 16-bit register, off the stack.  */
rtx
ia16_pop_reg (unsigned int regno)
{
  return gen_rtx_SET (VOIDmode, gen_rtx_REG (HImode, regno),
		      gen_frame_mem (HImode,
				     gen_rtx_POST_INC (Pmode,
						       stack_pointer_rtx)));
}

/* Trampolines for Nested Functions */
/* tr = trampoline addr, fn = function addr, sc = static chain */
/* FIXME: TARGET_CODE32 is not supported - add data16 and addr16 prefixes.  */
void
ia16_initialize_trampoline (rtx tr, rtx fn, rtx sc)
{
	rtx fn_disp;
	/* Opcode register encoding   sBDSbdac for sp bp di si bx dx ax cx */
	unsigned long int regtable = 045763201;

	/* Base opcode for movw $imm16, reg16.  */
	unsigned char mov_opcode = 0xb8;
	unsigned char regno = STATIC_CHAIN_REGNUM;

	if (TARGET_SEPARATE_CSEG)
	  {
	    sorry ("Trampolines are disabled by -mseparate-code-segment.");
	    abort ();
	  }

	if (TEST_HARD_REG_BIT (reg_class_contents[QI_REGS], regno))
		regno /= 2;
	else
		regno -= 4;
	mov_opcode |= (regtable >> (regno * 3)) & 7;

	/* Write the movw opcode.  */
	emit_move_insn (gen_rtx_MEM (QImode, tr),
			GEN_INT ((signed char) mov_opcode));

	/* Write the static chain value.  */
	emit_move_insn (gen_rtx_MEM (Pmode, plus_constant (tr, 1)), sc);

	/* Write the absolute jmp opcode.  */
	emit_move_insn (gen_rtx_MEM (QImode, plus_constant (tr, 3)),
	                GEN_INT ((signed char) 0xe9));

	/* Calculate the difference between the function address and %pc, which
	   points to (tr+4)+2 when the jmp instruction executes.  */
	fn_disp = expand_binop (Pmode, sub_optab, fn, plus_constant (tr, 4 + 2),
				NULL_RTX, 1, OPTAB_DIRECT);

	/* Write the jmp offset.  */
	emit_move_insn (gen_rtx_MEM (Pmode, plus_constant (tr, 4)), fn_disp);

	/* Done.  That wasn't a lot of fun.  */
}

/* Addressing Modes.  */
/* Check an address E and optionally split it into its components.
 */
bool
ia16_parse_address (rtx e, rtx *p_r1, rtx *p_r2, rtx *p_c)
{
	rtx x, y;
	rtx r1 = NULL, r2 = NULL, c = NULL;

	if (REG_P (e))
		r1 = e;
	else if (GET_CODE(e) == PLUS) {
		x = XEXP (e, 0);
		y = XEXP (e, 1);
		if (GET_CODE (x) == PLUS) {
			r1 = XEXP (x, 0);
			r2 = XEXP (x, 1);
			c  = y;
		} else if (REG_P (y)) {
			r1 = x;
			r2 = y;
		} else {
			r1 = x;
			c  = y;
		}
	} else {
		c = e;
	}

	/* Deal with the most obvious brokenness.  */
	if ((r1 && !REG_P (r1)) || (r2 && !REG_P (r2)) ||
            (c && !CONSTANT_P (c))) {
		return (0 == 1);
	}

	/* Check that we have at least a base or a displacement.  */
	if (!c && !r1)
		return (0 == 1);

	if (p_r1)
		*p_r1 = r1;
	if (p_r2)
		*p_r2 = r2;
	if (p_c)
		*p_c = c;

	return (0 == 0);
}

/* Miscellaneous Parameters */

/* Only Intel i80186 and i80286 mask the shift/rotate counts.  */
/* FIXME Documentation bug: unsigned HOST_WIDE_INT instead of int.  */
#undef  TARGET_SHIFT_TRUNCATION_MASK
#define TARGET_SHIFT_TRUNCATION_MASK	ia16_shift_truncation_mask

static unsigned HOST_WIDE_INT
ia16_shift_truncation_mask (enum machine_mode mode)
{
  return (TARGET_SHIFT_MASKED && (mode == HImode || mode == QImode) ? 31 : 0);
}

/* The Global targetm Variable */

/* #include "target.h" */
/* #include "target-def.h" */

/* Initialize the GCC target structure.  */
struct gcc_target targetm = TARGET_INITIALIZER;

/* Support functions not documented in the GCC internals manual.  */

/* Return true if the operands are suitable for an insn of code CODE.
 * Return false otherwise.  Note that we don't bother checking combinations
 * of register and immediate operands since the operand predicates should be
 * enough.  SCRATCH expressions are also accepted for operand 0.
 * The insn is assumed to have exactly three operands.
 */
bool ia16_arith_operands_p (enum rtx_code code, rtx *operands)
{
  if (MEM_P (operands[1]) && MEM_P (operands[2]))
    return (false);

  if (MEM_P (operands[0]))
    {
      /* OK if op0 and op1 is the same MEM.  */
      if (rtx_equal_p (operands[0], operands[1]))
	return (true);

      /* OK if op0 and op2 is the same MEM if
	 1) CODE is commutative, or
	 2) CODE is MINUS and op1 is zero.  */
      if ((RTX_COMM_ARITH == GET_RTX_CLASS (code)
	   || (MINUS == code
	      && operands[1] == CONST0_RTX (GET_MODE (operands[1]))))
	  && rtx_equal_p (operands[0], operands[2]))
	return (true);
    }
  else if ((RTX_COMM_ARITH == GET_RTX_CLASS (code)
	    && !CONSTANT_P (operands[2]))
	   || !MEM_P (operands[1]))
    return (true);
  return (false);
}

/* Mung OPERANDS enough to be accepted by ia16_arith_operands_p (CODE,
   OPERANDS).  If operands[0] needs to be munged, return a new pseudo register
   operand to use as a temporary.  Otherwise, return NULL.
   This function may create new pseudo registers.
   TODO: Three different memory operands are all forced to registers.  Ideally,
   only operands[0] and operands[1] should be forced to registers.
 */
rtx ia16_prepare_operands (enum rtx_code code, rtx *operands)
{
  rtx tmp = NULL;

  /* If op1 is MEM, op0 must be the same mem.  If they are the same mem, then
     op2 must not also be a mem.  */
  if (MEM_P (operands[1]))
    if (!rtx_equal_p (operands[0], operands[1]))
      operands[1] = force_reg (GET_MODE (operands[1]), operands[1]);
    else if (MEM_P (operands[2]))
      operands[2] = force_reg (GET_MODE (operands[2]), operands[2]);

  /* If op2 is MEM, op0 must be the same mem and CODE != MINUS.  */
  if (MEM_P (operands[2])
      && (RTX_BIN_ARITH == GET_RTX_CLASS (code)
	  || !rtx_equal_p (operands[0], operands[2])))
    operands[2] = force_reg (GET_MODE (operands[2]), operands[2]);

  /* If op0 is MEM different from op1 and op2, use a scratch register.  */
  if (MEM_P (operands[0])
      && (!MEM_P (operands[1]) || !MEM_P (operands[2])))
    tmp = gen_reg_rtx (GET_MODE (operands[0]));
  return (tmp);
}

/* Split a memory address displacement X into a symbolic part SYMVAL and an
 * integer part INTVAL.  X may be NULL.
 */
static void
ia16_parse_constant (rtx x, rtx *symval, rtx *intval)
{
  if (x == NULL)
    {
      *symval = NULL;
      *intval = const0_rtx;
    }
  else if (CONST_INT_P (x))
    {
      *symval = NULL;
      *intval = x;
    }
  /* Try to handle (const (plus (...) (const_int c))).  */
  else if (CONST == GET_CODE (x) && PLUS == GET_CODE (XEXP (x, 0)))
    {
      if (CONST_INT_P (XEXP (XEXP (x, 0), 1)))
	{
	  *symval = XEXP (XEXP (x, 0), 0);
	  *intval = XEXP (XEXP (x, 0), 1);
	}
      else
	{
	  *symval = XEXP (x, 0);
	  *intval = const0_rtx;
	}
    }
  else
    {
      *symval = x;
      *intval = const0_rtx;
    }
}

/* Return true if the addresses of memory operands M1 and M2 differ only by
 * a fixed, numeric value, and store the number of units of M1 - M2 in OFFSET.
 * Return false otherwise.  */
static bool
ia16_memory_offset_known (rtx m1, rtx m2, int *offset)
{
  rtx m1base, m1index, m1disp, m1sym, m1int;
  rtx m2base, m2index, m2disp, m2sym, m2int;

  ia16_parse_address_strict (XEXP (m1, 0), &m1base, &m1index, &m1disp);
  ia16_parse_address_strict (XEXP (m2, 0), &m2base, &m2index, &m2disp);

  if (!rtx_equal_p (m1base, m2base) || !rtx_equal_p (m1index, m2index))
    return (false);

  ia16_parse_constant (m1disp, &m1sym, &m1int);
  ia16_parse_constant (m2disp, &m2sym, &m2int);

  if (!rtx_equal_p (m1sym, m2sym))
    return (false);

  *offset = INTVAL (m1int) - INTVAL (m2int);
  return (true);
}

/* Return true if memory operands M1 and M2 are suitable for a move
 * multiple instruction in mode MODE.  Return false otherwise.  */
bool
ia16_move_multiple_mem_p (enum machine_mode mode, rtx m1, rtx m2)
{
  int offset;

  if (ia16_memory_offset_known (m2, m1, &offset))
    return (GET_MODE_SIZE (mode) == offset);
  else
    return (false);
}

/* Return true if register operands R1 and R2 are suitable for a move
 * multiple instruction in mode MODE.  Return false otherwise.  */
bool
ia16_move_multiple_reg_p (enum machine_mode mode, rtx r1, rtx r2)
{
  enum machine_mode mode2x = GET_MODE_2XWIDER_MODE (mode);
  unsigned int reg1no = REGNO (r1);

  if (!HARD_REGNO_MODE_OK (reg1no, mode2x))
    return (false);
  return (REGNO (r2) - reg1no
	  == subreg_regno_offset (reg1no, mode2x, GET_MODE_SIZE (mode), mode));
}

/* Return true if we can prove that memory operands M1 and M2 don't overlap.
 * Return false otherwise.  */
bool
ia16_non_overlapping_mem_p (rtx m1, rtx m2)
{
  int offset;

  if (!ia16_memory_offset_known (m1, m2, &offset))
    return (false);

  if (offset < 0)
    return (GET_MODE_SIZE (GET_MODE (m1)) <= -offset);
  else if (offset > 0)
    return (GET_MODE_SIZE (GET_MODE (m2)) <=  offset);
  else
    return (false);
}
