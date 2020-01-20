/* Machine-dependent reorganization pass for Intel 16-bit x86 target.
   Copyright (C) 2018-2020 Free Software Foundation, Inc.
   Contributed by TK Chia <https://github.com/tkchia/>

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "rtl.h"
#include "tree.h"
#include "df.h"
#include "tm_p.h"
#include "expmed.h"
#include "optabs.h"
#include "regs.h"
#include "emit-rtl.h"
#include "diagnostic.h"
#include "fold-const.h"
#include "calls.h"
#include "stor-layout.h"
#include "varasm.h"
#include "output.h"
#include "explow.h"
#include "expr.h"
#include "builtins.h"
#include "cfgrtl.h"
#include "libiberty.h"
#include "hashtab.h"
#include "print-tree.h"
#include "langhooks.h"
#include "stringpool.h"
#include "attribs.h"
#include "varasm.h"
#include "recog.h"

static int
ia16_ss_data_function_rtx_p (rtx addr)
{
  tree type = ia16_get_function_type_for_addr (addr);
  return ia16_ss_data_function_type_p (type);
}

/* If we are in an __attribute__ ((no_assume_ds_data)) function, and %ds is
   fixed, check whether it calls any __attribute__ ((assume_ds_data))
   functions.  Flag warnings if it does.  */
static void
ia16_verify_calls_from_no_assume_ds (void)
{
  rtx_insn *insn;

  if (TARGET_ALLOCABLE_DS_REG
      || ia16_in_ds_data_function_p ())
    return;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);
	  if (ia16_ds_data_function_rtx_p (callee))
	      warning_at (LOCATION_LOCUS (INSN_LOCATION (insn)),
			  OPT_Wmaybe_uninitialized,
			  "%%ds is fixed, not resetting %%ds for call to "
			  "assume_ds_data function");
	}
    }
}

/* If we are in an __attribute__ ((no_assume_ss_data)) function,
   check whether it calls any __attribute__ ((assume_ss_data)) functions. 
   Flag warnings if it does.  */
static void
ia16_verify_calls_from_no_assume_ss (void)
{
  rtx_insn *insn;

  if (ia16_in_ss_data_function_p ())
    return;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);
	  if (ia16_ss_data_function_rtx_p (callee))
	      warning_at (LOCATION_LOCUS (INSN_LOCATION (insn)),
			  OPT_Wmaybe_uninitialized,
			  "calling assume_ss_data function when "
			  "%%ss may not point to data segment");
	}
    }
}

/* Say whether an RTX operand is the %bp register.  */
static bool
ia16_operand_is_bp_reg_p (rtx op)
{
  return REG_P (op) && REGNO (op) == BP_REG && GET_MODE (op) == HImode;
}

/* Say whether an RTX operand is the %sp register.  */
static bool
ia16_operand_is_sp_reg_p (rtx op)
{
  return REG_P (op) && REGNO (op) == SP_REG && GET_MODE (op) == HImode;
}

/* Say whether an RTX insn pattern is a (clobber (reg CC_REG)).  */
static bool
ia16_pattern_is_clobber_cc_reg_p (rtx pat)
{
  rtx op;
  if (GET_CODE (pat) != CLOBBER)
    return false;
  op = SET_DEST (pat);
  return REG_P (op) && REGNO (op) == CC_REG;
}

/* Say whether an insn is a `pushw %bp' in a prologue.  */
static bool
ia16_insn_is_prologue_pushw_bp_p (rtx_insn *insn)
{
  rtx pat, pat0;

  if (! INSN_P (insn)
      || ! RTX_FRAME_RELATED_P (insn)
      || INSN_CODE (insn) != CODE_FOR__pushhi1_nonimm)
    return false;

  pat = PATTERN (insn);
  gcc_assert (GET_CODE (pat) == PARALLEL);

  pat0 = XVECEXP (pat, 0, 0);
  gcc_assert (GET_CODE (pat0) == SET);

  return ia16_operand_is_bp_reg_p (SET_SRC (pat0));
}

/* Say whether an insn is a `movw %sp, %bp' in a prologue.  */
static bool
ia16_insn_is_prologue_movw_sp_bp_p (rtx_insn *insn)
{
  rtx pat = PATTERN (insn);

  return INSN_P (insn)
	 && RTX_FRAME_RELATED_P (insn)
	 && GET_CODE (pat) == SET
	 && ia16_operand_is_bp_reg_p (SET_DEST (pat))
	 && ia16_operand_is_sp_reg_p (SET_SRC (pat));
}

/* Say whether an insn is a `subw $CONST_INT, %sp' in a prologue.  If so,
   also say what the CONST_INT is.  */
static bool
ia16_insn_is_prologue_subw_const_sp_p (rtx_insn *insn, HOST_WIDE_INT *p_value)
{
  rtx pat, pat0, pat0_src, pat1;

  if (! INSN_P (insn) || ! RTX_FRAME_RELATED_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != PARALLEL || XVECLEN (pat, 0) != 2)
    return false;

  pat0 = XVECEXP (pat, 0, 0);
  pat1 = XVECEXP (pat, 0, 1);

  if (GET_CODE (pat0) != SET
      || ! ia16_operand_is_sp_reg_p (SET_DEST (pat0))
      || ! ia16_pattern_is_clobber_cc_reg_p (pat1))
    return false;

  pat0_src = SET_SRC (pat0);
  if (GET_CODE (pat0_src) != MINUS
      || ! ia16_operand_is_sp_reg_p (XEXP (pat0_src, 0))
      || ! CONST_INT_P (XEXP (pat0_src, 1)))
    return false;

  *p_value = INTVAL (XEXP (pat0_src, 1));
  return true;
}

/* Say whether an insn is a `popw %bp'.  */
static bool
ia16_insn_is_popw_bp_p (rtx_insn *insn)
{
  rtx pat;

  if (! INSN_P (insn) || INSN_CODE (insn) != CODE_FOR__pophi1)
    return false;
  pat = PATTERN (insn);
  gcc_assert (GET_CODE (pat) == SET);
  return ia16_operand_is_bp_reg_p (SET_DEST (pat));
}

/* Say whether an RTX corresponds to an expression which performs arithmetic
   on registers without altering them.  */
static bool
ia16_rtx_is_pure_reg_arith_p (rtx x)
{
  switch (GET_RTX_CLASS (GET_CODE (x)))
    {
    case RTX_OBJ:
      return REG_P (x);

    case RTX_CONST_OBJ:
      return true;

    case RTX_UNARY:
      return ia16_rtx_is_pure_reg_arith_p (XEXP (x, 0));

    case RTX_COMPARE:
    case RTX_COMM_COMPARE:
    case RTX_COMM_ARITH:
    case RTX_BIN_ARITH:
      return ia16_rtx_is_pure_reg_arith_p (XEXP (x, 0))
	     && ia16_rtx_is_pure_reg_arith_p (XEXP (x, 1));

    case RTX_BITFIELD_OPS:
    case RTX_TERNARY:
      return ia16_rtx_is_pure_reg_arith_p (XEXP (x, 0))
	     && ia16_rtx_is_pure_reg_arith_p (XEXP (x, 1))
	     && ia16_rtx_is_pure_reg_arith_p (XEXP (x, 2));

    default:
      return false;
    }
}

/* Say whether an RTX (in the currently compiled function) is for a memory
   operand in the program's default data segment.  */
static bool
ia16_data_seg_mem_p (rtx x)
{
  return MEM_P (x) && ADDR_SPACE_GENERIC_P (MEM_ADDR_SPACE (x));
}

/* Say whether an RTX (in the currently compiled function) is for a memory
   operand in the same IA-16 segment as the stack.  */
static bool
ia16_stack_seg_mem_p (rtx x)
{
  if (! MEM_P (x))
    return false;

  switch (MEM_ADDR_SPACE (x))
    {
    case ADDR_SPACE_GENERIC:
      return ia16_in_ss_data_function_p () ? true : false;

    case ADDR_SPACE_SEG_SS:
      return true;

    default:
      return false;
    }
}

/* In our reorganization subpasses, we sometimes need to look for pairs of
   adjacent instructions --- within the same basic block, and possibly only
   separated by NOTE's.  Define a macro FOR_EACH_IA16_INSN_PAIR to help in
   this.

   When using this macro, the basic block information for each of the insns
   should be set correctly.  */
static void
insn_pair_iterator_next (rtx_insn **p_insn1, rtx_insn **p_insn2)
{
  rtx_insn *insn1, *insn2 = *p_insn2;

  gcc_assert (insn2);

  do
    {
      insn1 = insn2;
      insn2 = NEXT_INSN (insn1);

      if (insn1 && BLOCK_FOR_INSN (insn1)
	  && insn1 == BB_END (BLOCK_FOR_INSN (insn1)))
	insn1 = NULL;

      while (insn2 && ! INSN_P (insn2))
	{
	  if (! NOTE_P (insn2))  /* probably a BARRIER or CODE_LABEL */
	    insn1 = NULL;
	  else if (BLOCK_FOR_INSN (insn2)
		   && insn2 == BB_END (BLOCK_FOR_INSN (insn2)))
	    insn1 = NULL;
	  insn2 = NEXT_INSN (insn2);
	}
    }
  while (! insn1 && insn2);

  *p_insn1 = insn1;
  *p_insn2 = insn2;
}

static void
insn_pair_iterator_init (rtx_insn **p_insn1, rtx_insn **p_insn2)
{
  rtx_insn *insn2 = get_insns ();

  while (insn2 && ! INSN_P (insn2))
    insn2 = NEXT_INSN (insn2);

  *p_insn1 = NULL;
  *p_insn2 = insn2;
  if (insn2)
    insn_pair_iterator_next (p_insn1, p_insn2);
}

#define FOR_EACH_IA16_INSN_PAIR(insn1, insn2) \
	for (insn_pair_iterator_init (&(insn1), &(insn2)); \
	     (insn2); \
	     insn_pair_iterator_next (&(insn1), &(insn2)))

/* Try to rewrite something like
		movw	%sp,	%bp
		subw	$6,	%sp
		...				// %sp and %bp unchanged
		movw	%cx,	-6(%bp)		// %cx unchanged
		...
		movw	%dx,	-4(%bp)		// %dx unchanged
   into something like
		movw	%sp,	%bp
		subw	$2,	%sp		// or `pushw %ds'
		pushw	%dx
		pushw	%cx
		...
 */
static void
ia16_rewrite_reg_parm_save_as_push (void)
{
  basic_block bb;
  rtx_insn *insn1, *insn2, *insn;

  FOR_EACH_IA16_INSN_PAIR (insn1, insn2)
    {
      enum
	{
	  /* The maximum offset from %sp for which we want to track the
	     stack slot contents.  */
	  MAX_TRACK_SP_OFF = 2 * LAST_ALLOCABLE_REG,
	  /* Bogus register number meaning "this stack slot is not filled
	     yet".  */
	  UNFILLED = FIRST_PSEUDO_REGISTER,
	  /* Bogus register number meaning "this stack slot is filled with
	     something, but we are not sure what".  */
	  UNKNOWN = FIRST_PSEUDO_REGISTER + 1
	};
      rtx pat, dest, dest_addr, src, sp_reg;
      HOST_WIDE_INT bp_sp_off = 0, new_bp_sp_off;
      unsigned sp_off_contents[MAX_TRACK_SP_OFF / 2 + 1];
      rtx_insn *sp_off_insn[MAX_TRACK_SP_OFF / 2 + 1];
      bool pristine[LAST_ALLOCABLE_REG + 1];
      unsigned regno, index;
      machine_mode mode;

      /* Look for the sequence `movw %sp, %bp; subw $CONST_INT, %sp' in a
	 prologue.  Bail out if this sequence is missing, if the CONST_INT
	 is not sane, or if we encounter an instruction which might throw
	 exceptions.  */
      if (insn_could_throw_p (insn2)
	  || ! ia16_insn_is_prologue_movw_sp_bp_p (insn1)
	  || ! ia16_insn_is_prologue_subw_const_sp_p (insn2, &bp_sp_off))
	continue;

      if (bp_sp_off < 2)
	continue;

      /* We found the sequence.  Scan through any simple register assignments
	 and/or simple arithmetic operations immediately following the `subw
	 $..., %sp'.

	 Record the places where the original values of registers are stored
	 in the stack slots we are tracking.

	 TODO: handle more instruction types.  */
      for (regno = 0; regno <= LAST_ALLOCABLE_REG; ++regno)
        pristine[regno] = true;

      /* (INDEX is really an offset from %sp, divided by 2...)  */
      for (index = 0; index <= MAX_TRACK_SP_OFF / 2; ++index)
	{
	  sp_off_contents[index] = UNFILLED;
	  sp_off_insn[index] = NULL;
	}

      insn = insn2;
      bb = BLOCK_FOR_INSN (insn);
      while (insn != BB_END (bb))
	{
	  insn = NEXT_INSN (insn);
	  if (! INSN_P (insn))
	    continue;

	  if (insn_could_throw_p (insn))
	    break;

	  pat = PATTERN (insn);
	  if (GET_CODE (pat) == PARALLEL)
	    {
	      if (XVECLEN (pat, 0) != 2
		  || ! ia16_pattern_is_clobber_cc_reg_p (XVECEXP (pat, 0, 1)))
		break;
	      pat = XVECEXP (pat, 0, 0);
	    }

	  if (GET_CODE (pat) != SET)
	    break;

	  dest = SET_DEST (pat);
	  src = SET_SRC (pat);
	  mode = GET_MODE (dest);

	  if (mode != HImode && mode != QImode
	      && mode != PHImode && mode != V2QImode)
	    break;

	  if (! ia16_rtx_is_pure_reg_arith_p (src))
	    break;

	  if (REG_P (dest))
	    {
	      regno = REGNO (dest);
	      if (regno > LAST_ALLOCABLE_REG || regno == BP_REG)
		break;

	      pristine[regno] = false;
	      if (regno < FIRST_NOQI_REG && mode != QImode)
		pristine[regno + 1] = false;
	    }
	  else if (ia16_stack_seg_mem_p (dest))
	    {
	      dest_addr = XEXP (dest, 0);
	      if (GET_CODE (dest_addr) == PLUS
		  && CONST_INT_P (XEXP (dest_addr, 1))
		  && ia16_operand_is_bp_reg_p (XEXP (dest_addr, 0)))
		{
		  /* We found a
			movw	%reg,	CONST_INT(%bp)
		     or
			movb	%reg,	CONST_INT(%bp)
		     insn.  Convert the offset from %bp into an offset from
		     the top of the stack.  And, if %reg still has its
		     initial value at this point, note down that the stack
		     slot contains this value.  */
		  HOST_WIDE_INT sp_operand_off
				  = bp_sp_off + INTVAL (XEXP (dest_addr, 1));

		  if (sp_operand_off % 2 != 0)
		    break;

		  if (sp_operand_off < 0  /* ??? */
		      || sp_operand_off > MAX_TRACK_SP_OFF
		      || sp_operand_off > bp_sp_off - 2)
		    continue;

		  if (sp_off_contents[sp_operand_off / 2] != UNFILLED)
		    break;

		  if (REG_P (src)
		      && REGNO (src) <= LAST_ALLOCABLE_REG
		      && pristine[REGNO (src)]
		      && ! ia16_regno_in_class_p (REGNO (src), UP_QI_REGS)
		      && (mode == QImode || REGNO (src) >= FIRST_NOQI_REG
			  || pristine[REGNO (src) + 1]))
		    {
		      sp_off_contents[sp_operand_off / 2] = REGNO (src);
		      sp_off_insn[sp_operand_off / 2] = insn;
		    }
		  else
		    sp_off_contents[sp_operand_off / 2] = UNKNOWN;
		}
	      else
		break;
	    }
	  else
	    break;
	}

      /* We can now try to replace the insns.  */
      index = 0;
      while (index <= MAX_TRACK_SP_OFF / 2
	     && sp_off_contents[index] != UNFILLED
	     && sp_off_contents[index] != UNKNOWN)
	++index;

      if (! index)
	continue;

      insn = insn2;
      new_bp_sp_off = bp_sp_off - 2 * index;
      sp_reg = SET_DEST (XVECEXP (PATTERN (insn), 0, 0));

      if (new_bp_sp_off)
	{
	  rtx pat0, pat1;
	  pat0 = gen_rtx_SET (sp_reg,
			      gen_rtx_MINUS (HImode, sp_reg,
					    GEN_INT (new_bp_sp_off)));
	  pat1 = gen_hard_reg_clobber (CCmode, CC_REG);
	  PATTERN (insn) = gen_rtx_PARALLEL (VOIDmode,
					     gen_rtvec (2, pat0, pat1));
	  ia16_recog (insn);
	}
      else
	{
	  /* We can completely elide the %sp adjustment.  Instead of emitting
	     a `subw $0, %sp' insn, replace it with a dummy (use ...).  */
	  PATTERN (insn) = gen_rtx_USE (HImode, sp_reg);
	  INSN_CODE (insn) = -1;
	  RTX_FRAME_RELATED_P (insn) = 0;
	}

      while (index != 0)
	{
	  rtx reg;
	  rtx_insn *push_reg_insn, *mov_reg_insn;

	  --index;

	  /* Build and insert the new `pushw' insn.  */
	  reg = gen_rtx_REG (HImode, sp_off_contents[index]);
	  push_reg_insn = make_insn_raw (gen__pushhi1_nonimm (reg));
	  RTX_FRAME_RELATED_P (push_reg_insn) = 1;
	  ia16_recog (push_reg_insn);
	  add_insn_after (push_reg_insn, insn, bb);

	  /* Blank out the original `mov{w|b} %reg, ...(%bp)' insn.  */
	  mov_reg_insn = sp_off_insn[index];
	  PATTERN (mov_reg_insn) = gen_rtx_USE (HImode, reg);
	  INSN_CODE (mov_reg_insn) = -1;
	  RTX_FRAME_RELATED_P (mov_reg_insn) = 0;

	  insn = push_reg_insn;
	}

      /* The basic block information should still be correct, so we can
	 simply continue iterating over the insn chain.  */
    }
}

/* Try to rewrite the current function so that instead of using the frame
   pointer register, %bp, it uses %bx.  This can potentially allow the
   function to preserve one less register --- unlike %bp, %bx is call-used.
 */
static void
ia16_rewrite_bp_as_bx (void)
{
  edge e;
  edge_iterator ei;
  rtx_insn *insn, *pushw_bp_insn = NULL, *movw_sp_bp_insn = NULL, **stack;
  rtx sp_reg, bx_reg, use_sp, movw_sp_bx, *rewrite_insn_pat;
  int max_insn_uid, uid, sp = 0;
  bool bp_used = false;
  typedef enum
    {
      RBPS_UNKNOWN,
      RBPS_BP_GOOD,
      RBPS_BX_CLOBBERED,
      RBPS_BP_POPPED
    } rewrite_bp_state_t;
  rewrite_bp_state_t *insn_state, state;
  basic_block bb;

  /* Bail out early if we are not supposed to eliminate the frame pointer,
     or if the function does not use %bp anyway, or in the unlikely future
     case that we actually pass function arguments via %bx.  */
  if (! global_options.x_flag_omit_frame_pointer
      || ! ia16_save_reg_p (BP_REG)
      || FUNCTION_ARG_REGNO_P (B_REG)
      || FUNCTION_ARG_REGNO_P (BH_REG))
    return;

  /* For now, do not try to rewrite functions which may throw exceptions ---
     until we can figure out how to do the rewriting correctly.

     TODO: allow rewriting functions marked __attribute__ ((nothrow)) etc.  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    if (INSN_P (insn) && insn_could_throw_p (insn))
      return;

  /* Look for the `pushw %bp; movw %sp, %bp' stack frame setup sequence in
     the very first basic block.

     Bail out if this sequence is missing, or if the function frobs %bp or %bx
     in a different way.

     Also, to simplify the logic here, and save some time --- at the expense
     of missing some optimization opportunities --- do not try to look for
     this insn beyond the first basic block.  */
  if (EDGE_COUNT (ENTRY_BLOCK_PTR_FOR_FN (cfun)->succs) != 1)
    return;

  FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR_FOR_FN (cfun)->succs)
    {
      bb = e->dest;

      for (insn = BB_HEAD (bb); insn != BB_END (bb); insn = NEXT_INSN (insn))
	{
	  if (! INSN_P (insn))
	    continue;

	  if (dead_or_set_regno_p (insn, B_REG)
	      || dead_or_set_regno_p (insn, BH_REG)
	      || dead_or_set_regno_p (insn, BP_REG))
	    return;

	  if (ia16_insn_is_prologue_pushw_bp_p (insn)
	      && ia16_insn_is_prologue_movw_sp_bp_p (NEXT_INSN (insn)))
	    {
	      pushw_bp_insn = insn;
	      movw_sp_bp_insn = NEXT_INSN (insn);
	      break;
	    }
	}
    }

  if (! pushw_bp_insn)
    return;

  /* Do a depth-first search to decide if we want to rewrite this function's
     insns, and how.  */
  max_insn_uid = get_max_uid ();
  insn_state = XCNEWVEC (rewrite_bp_state_t, max_insn_uid);
  stack = XCNEWVEC (rtx_insn *, max_insn_uid);
  rewrite_insn_pat = XCNEWVEC (rtx, max_insn_uid);

  sp_reg = gen_rtx_REG (HImode, SP_REG);
  bx_reg = gen_rtx_REG (HImode, B_REG);
  use_sp = gen_rtx_USE (HImode, sp_reg);

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      uid = INSN_UID (insn);
      insn_state[uid] = RBPS_UNKNOWN;
      rewrite_insn_pat[uid] = NULL_RTX;
    }

  uid = INSN_UID (pushw_bp_insn);
  rewrite_insn_pat[uid] = use_sp;

  stack[sp++] = movw_sp_bp_insn;
  uid = INSN_UID (movw_sp_bp_insn);
  insn_state[uid] = RBPS_BP_GOOD;
  movw_sp_bx = gen_rtx_SET (bx_reg, sp_reg);
  RTX_FRAME_RELATED_P (movw_sp_bx) = 1;
  rewrite_insn_pat[uid] = movw_sp_bx;

  while (sp != 0)
    {
      rtx pat;

      insn = stack[--sp];
      pat = PATTERN (insn);
      uid = INSN_UID (insn);
      state = insn_state[uid];

      switch (state)
	{
	case RBPS_BP_GOOD:
	  if (insn == movw_sp_bp_insn || ! INSN_P (insn))
	    break;

	  if (RTX_FRAME_RELATED_P (insn))
	    goto bail;

	  if (refers_to_regno_p (BP_REG, BP_REG + 1, pat, NULL))
	    {
	      rtx dest, src, addr;

	      bp_used = true;
	      if (GET_CODE (pat) == SET)
		{
		  /* Deal with the common case of a (set ...) insn involving
		     stack arguments or stack local variables.  */
		  dest = SET_DEST (pat);
		  src = SET_SRC (pat);

		  if (REG_P (dest) && REGNO (dest) != BP_REG
		      && ia16_stack_seg_mem_p (src))
		    {
		      addr = XEXP (src, 0);
		      if (GET_CODE (addr) == PLUS
			  && ia16_operand_is_bp_reg_p (XEXP (addr, 0))
			  && CONST_INT_P (XEXP (addr, 1)))
			{
			  HOST_WIDE_INT off = INTVAL (XEXP (addr, 1));
			  if (off >= 2)
			    off -= 2;
			  else if (off >= 0)
			    goto bail;

			  addr = gen_rtx_PLUS (GET_MODE (addr), bx_reg,
					       GEN_INT (off));
			  src = gen_rtx_MEM (GET_MODE (src), addr);
			  rewrite_insn_pat[uid] = gen_rtx_SET (dest, src);
			}
		      else
			goto bail;

		      if (REGNO (dest) == B_REG || REGNO (dest) == BH_REG)
			state = RBPS_BX_CLOBBERED;
		    }
		  else if (ia16_stack_seg_mem_p (dest)
			   && REG_P (src) && REGNO (src) != BP_REG
			   && REGNO (src) != B_REG && REGNO (src) != BH_REG)
		    {
		      addr = XEXP (dest, 0);
		      if (GET_CODE (addr) == PLUS
			  && ia16_operand_is_bp_reg_p (XEXP (addr, 0))
			  && CONST_INT_P (XEXP (addr, 1)))
			{
			  HOST_WIDE_INT off = INTVAL (XEXP (addr, 1));
			  if (off >= 2)
			    off -= 2;
			  else if (off >= 0)
			    goto bail;

			  addr = gen_rtx_PLUS (GET_MODE (addr), bx_reg,
					       GEN_INT (off));
			  dest = gen_rtx_MEM (GET_MODE (dest), addr);
			  rewrite_insn_pat[uid] = gen_rtx_SET (dest, src);
			}
		      else
			goto bail;
		    }
		  else
		    goto bail;
		}
	      else
		goto bail;
	    }
	  else if (dead_or_set_regno_p (insn, BP_REG))
	    {
	      if (ia16_insn_is_popw_bp_p (insn))
		{
		  /* `popw %bp'.  */
		  rewrite_insn_pat[uid] = use_sp;
		  state = RBPS_BP_POPPED;
		}
	      else if (INSN_CODE (insn) == CODE_FOR__leave)
		{
		  /* `leave'.  */
		  rewrite_insn_pat[uid] = gen_rtx_SET (sp_reg, bx_reg);
		  state = RBPS_BP_POPPED;
		}
	      else
		goto bail;
	    }
	  else if (refers_to_regno_p (B_REG, BH_REG + 1, pat, NULL))
	    goto bail;
	  else if (dead_or_set_regno_p (insn, B_REG)
		   || dead_or_set_regno_p (insn, BH_REG))
	    state = RBPS_BX_CLOBBERED;

	  break;

	case RBPS_BX_CLOBBERED:
	  if (ia16_insn_is_popw_bp_p (insn))
	    {
	      state = RBPS_BP_POPPED;
	      rewrite_insn_pat[uid] = use_sp;
	      break;
	    }

	  /* fall through */
	case RBPS_BP_POPPED:
	  if (INSN_P (insn)
	      && (refers_to_regno_p (BP_REG, BP_REG + 1, pat, NULL)
		  || dead_or_set_regno_p (insn, BP_REG)))
	    goto bail;

	  break;

	default:
	  gcc_unreachable ();
	}

      bb = BLOCK_FOR_INSN (insn);
      if (insn != BB_END (bb))
	{
	  insn = NEXT_INSN (insn);
	  uid = INSN_UID (insn);

	  if (insn_state[uid] == RBPS_UNKNOWN)
	    {
	      insn_state[uid] = state;
	      stack[sp++] = insn;
	    }
	  else if (insn_state[uid] != state)
	    goto bail;
	}
      else
	{
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    {
	      if (e->dest == EXIT_BLOCK_PTR_FOR_FN (cfun))
		{
		  if (state != RBPS_BP_POPPED)
		    goto bail;
		  continue;
		}

	      insn = BB_HEAD (e->dest);
	      uid = INSN_UID (insn);

	      if (insn_state[uid] == RBPS_UNKNOWN)
		{
		  insn_state[uid] = state;
		  stack[sp++] = insn;
		}
	      else if (insn_state[uid] != state)
		goto bail;
	    }
	}
    }

  /* It looks like we can rewrite this function, so do it.  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      rtx pat;
      uid = INSN_UID (insn);
      pat = rewrite_insn_pat[uid];
      if (pat)
	{
	  PATTERN (insn) = pat;
	  ia16_recog (insn);
	  RTX_FRAME_RELATED_P (insn) = RTX_FRAME_RELATED_P (pat);
	}
    }

  /* If %bp is never used, we can also eliminate the `movw %sp, %bx'...  */
  if (! bp_used)
    {
      PATTERN (movw_sp_bp_insn) = use_sp;
      INSN_CODE (movw_sp_bp_insn) = -1;
      RTX_FRAME_RELATED_P (movw_sp_bp_insn) = 0;
    }

bail:
  free (insn_state);
  free (stack);
  free (rewrite_insn_pat);
}

/* Return true iff INSN is a (set (reg:SEG SOME_REG) (reg:SEG SS_REG)) where
   SOME_REG is a general register.  */
static bool
ia16_insn_is_copy_ss_p (rtx_insn *insn)
{
  rtx pat, op;

  if (! NONJUMP_INSN_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != SET)
    return false;

  op = SET_SRC (pat);
  if (! REG_P (op) || REGNO (op) != SS_REG || GET_MODE (op) != SEGmode)
    return false;

  op = SET_DEST (pat);
  if (! REG_P (op) || ! ia16_regno_in_class_p (REGNO (op), GENERAL_REGS)
      || GET_MODE (op) != SEGmode)
    return false;

  return true;
}

/* Return true iff INSN is a (set (reg:SEG DS_REG) (reg:SEG SS_REG)).  */
static bool
ia16_insn_is_reset_ds_p (rtx_insn *insn)
{
  rtx pat, op;

  if (! NONJUMP_INSN_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != SET)
    return false;

  op = SET_DEST (pat);
  if (! REG_P (op) || REGNO (op) != DS_REG || GET_MODE (op) != SEGmode)
    return false;

  op = SET_SRC (pat);
  if (! REG_P (op) || REGNO (op) != SS_REG || GET_MODE (op) != SEGmode)
    return false;

  return true;
}

/* Return true iff INSN is a (use (reg:SEG DS_REG)).  */
static bool
ia16_insn_is_use_ds_p (rtx_insn *insn)
{
  rtx pat, op;

  if (! NONJUMP_INSN_P (insn))
    return false;

  pat = PATTERN (insn);
  if (GET_CODE (pat) != USE)
    return false;

  op = XEXP (pat, 0);
  if (! REG_P (op) || REGNO (op) != DS_REG || GET_MODE (op) != SEGmode)
    return false;

  return true;
}

/* Elide unneeded instances of (set (reg:SEG DS_REG) (reg:SEG SS_REG)) in
   the current function.  Also elide any unneeded `%ss:' segment overrides
   from simple moves to/from memory (`ldsw', `cmpw', etc. are not yet
   handled).

   This routine should only be called on functions which assume %ss == .data .
 */
static void
ia16_elide_unneeded_ss_stuff (void)
{
  int max_insn_uid = get_max_uid ();
  bool *insn_pushed_p = XCNEWVEC (bool, max_insn_uid);
  bool *keep_insn_p = XCNEWVEC (bool, max_insn_uid);
  rtx_insn *insn, **stack = XCNEWVEC (rtx_insn *, max_insn_uid);
  int sp = 0;
  rtx ds = gen_rtx_REG (SEGmode, DS_REG),
      override = ia16_seg_override_term (ds);
  bool default_ds_abi_p = ia16_in_ds_data_function_p ();
  bool keep_any_resets_p = false;
  edge e;
  edge_iterator ei;

  /* Starting from insns which set (or clobber) %ds to not-.data, "flood"
     insns along the control flow until we hit
	(set (reg:SEG DS_REG) (reg:SEG SS_REG)).
     Assume %ss == .data always.

     If the current function does not assume %ds == .data on entry, also
     flood from the function entry point.

     Mark all the insns along each path --- excluding the first set to
     non-.data, but including the final %ds <- .data --- as insns we want to
     "keep" as is.  */
  if (! default_ds_abi_p)
    {
      FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR_FOR_FN (cfun)->succs)
	{
	  insn = BB_HEAD (e->dest);
	  if (! insn_pushed_p[INSN_UID (insn)])
	    {
	      stack[sp++] = insn;
	      insn_pushed_p[INSN_UID (insn)] = true;
	    }
	}
    }

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      /* GCC's machine-independent code thinks function calls always clobber
	 %ds, but we know that this is not really true.  */
      if (CALL_P (insn))
	{
	  rtx call = get_call_rtx_from (insn);
	  rtx callee = XEXP (XEXP (call, 0), 0);
	  if (ia16_ds_data_function_rtx_p (callee))
	    continue;
	}

      if (NOTE_P (insn)
	  || ! reg_set_p (ds, insn)
	  || ia16_insn_is_reset_ds_p (insn))
	continue;

      if (! insn_pushed_p[INSN_UID (insn)])
	{
	  stack[sp++] = insn;
	  insn_pushed_p[INSN_UID (insn)] = true;
	}
    }

  while (sp != 0)
    {
      rtx_insn *insn;
      basic_block bb;

      gcc_assert (sp > 0 && sp <= max_insn_uid);
      insn = stack[--sp];

      if (ia16_insn_is_reset_ds_p (insn))
	{
	  keep_any_resets_p = true;
	  continue;
	}

      bb = BLOCK_FOR_INSN (insn);
      if (insn != BB_END (bb))
	{
	  insn = NEXT_INSN (insn);
	  keep_insn_p[INSN_UID (insn)] = true;
	  if (! insn_pushed_p[INSN_UID (insn)])
	    {
	      stack[sp++] = insn;
	      insn_pushed_p[INSN_UID (insn)] = true;
	    }
	}
      else
	{
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    {
	      if (e->dest == EXIT_BLOCK_PTR_FOR_FN (cfun))
		continue;

	      insn = BB_HEAD (e->dest);
	      keep_insn_p[INSN_UID (insn)] = true;
	      if (! insn_pushed_p[INSN_UID (insn)])
		{
		  stack[sp++] = insn;
		  insn_pushed_p[INSN_UID (insn)] = true;
		}
	    }
	}
    }

  /* Rescan the whole insn list for
	(set (reg:SEG DS_REG) (reg:SEG SS_REG))
     and simple moves to/from the generic address space, and rewrite those
     which we have not marked as "keep".  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      rtx pat, dest, src;

      if (keep_insn_p[INSN_UID (insn)])
	continue;

      if (ia16_insn_is_reset_ds_p (insn))
	{
	  /* Rewrite
		(set (reg:SEG DS_REG) (reg:SEG SS_REG))
	     as
		(use (reg:SEG SS_REG)).

	     If this insn is followed by
		(use (reg:SEG DS_REG))
	     then rewrite that too.  */
	  rtx ss = XEXP (PATTERN (insn), 1);
	  PATTERN (insn) = gen_rtx_USE (VOIDmode, ss);
	  INSN_CODE (insn) = -1;

	  if (insn != BB_END (BLOCK_FOR_INSN (insn)))
	    {
	      rtx_insn *next_insn = NEXT_INSN (insn);
	      if (ia16_insn_is_use_ds_p (next_insn))
		XEXP (PATTERN (next_insn), 0) = ss;
	    }

	  continue;
	}

      if (! NONJUMP_INSN_P (insn) || GET_CODE (PATTERN (insn)) != SET)
	continue;

      pat = PATTERN (insn);
      dest = SET_DEST (pat);
      src = SET_SRC (pat);

      if (ia16_data_seg_mem_p (dest) && (REG_P (src) || CONSTANT_P (src)))
	{
	  /* If the insn is a simple move to generic memory, and the address
	     does not involve %bp, then _add_ a `%ds:' segment override to
	     the destination address term.

	     (These added overrides are not counted towards determining
	      whether %ds is ever live (below).  As Obi-Wan never said:
	      "These are not the %ds references you are looking for.")  */
	  rtx addr = XEXP (dest, 0), rb, rs;

	  if (ia16_parse_address_strict (addr, &rb, NULL, NULL, &rs,
					 ADDR_SPACE_GENERIC)
	      && ! rs && (! rb || REGNO (rb) != BP_REG))
	    {
	      rtx new_dest = shallow_copy_rtx (dest);
	      XEXP (new_dest, 0) = gen_rtx_PLUS (HImode, override, addr);
	      PATTERN (insn) = gen_rtx_SET (new_dest, src);
	      ia16_recog (insn);
	    }
	}
      else if (ia16_data_seg_mem_p (src) && REG_P (dest))
	{
	  /* Ditto if the insn is a simple move from generic memory.  */
	  rtx addr = XEXP (src, 0), rb, rs;

	  if (ia16_parse_address_strict (addr, &rb, NULL, NULL, &rs,
					 ADDR_SPACE_GENERIC)
	      && ! rs && (! rb || REGNO (rb) != BP_REG))
	    {
	      rtx new_src = shallow_copy_rtx (src);
	      XEXP (new_src, 0) = gen_rtx_PLUS (HImode, override, addr);
	      PATTERN (insn) = gen_rtx_SET (dest, new_src);
	      ia16_recog (insn);
	    }
	}
    }

  /* If at no point in the function do we need to reset %ds to .data, then
     deduce that %ds is no longer live within the function.  */
  if (default_ds_abi_p && ! keep_any_resets_p)
    df_set_regs_ever_live (DS_REG, false);

  free (insn_pushed_p);
  free (keep_insn_p);
  free (stack);
}

/* Try to rewrite
		movw	%ss,	%reg
		pushw	%ss
		popw	%ds
   as
		movw	%ss,	%reg
		movw	%reg,	%ds

   We cannot do this as a peephole rule, since we want to do it _after_ the
   above subpass to eliminate any (set (reg:SEG DS_REG) (reg:SEG SS_REG))
   patterns which are totally redundant in the first place.

   This routine should only be called on functions which assume %ss == .data .
 */
static void
ia16_tweak_reset_ds (void)
{
  rtx_insn *insn1, *insn2;

  FOR_EACH_IA16_INSN_PAIR (insn1, insn2)
    {
      rtx dest, new_src;

      if (! ia16_insn_is_copy_ss_p (insn1)
	  || ! ia16_insn_is_reset_ds_p (insn2))
	continue;

      new_src = SET_DEST (PATTERN (insn1));
      dest = SET_DEST (PATTERN (insn2));
      PATTERN (insn2) = gen_rtx_SET (dest, new_src);
      ia16_recog (insn2);
    }
}

/* If optimizing for size, try to rewrite each 2-byte `movw %reg, %ax' or
   `movw %ax, %reg' into a 1-byte `xchgw %ax, %reg', wherever possible.

   This optimization needs to be done as late as possible --- if it is in,
   say, the peephole2 pass, the resulting insn patterns for `xchgw' may
   interfere with subsequent passes.  */
static void
ia16_rewrite_movw_as_xchgw (void)
{
  bool reg_now_clobbered_p[LAST_ALLOCABLE_REG + 1];
  rtx_insn *insn;
  rtx pat, dest, src;
  unsigned rd, rs;

  for (rs = 0; rs <= LAST_ALLOCABLE_REG; ++rs)
    reg_now_clobbered_p[rs] = false;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (! INSN_P (insn))
	continue;

      pat = PATTERN (insn);
      if (GET_CODE (pat) != SET)
	continue;

      dest = SET_DEST (pat);
      src = SET_SRC (pat);
      if ((! REG_P (dest) && ! SUBREG_P (dest))
	  || (! REG_P (src) && ! SUBREG_P (src))
	  || GET_MODE_SIZE (GET_MODE (dest)) != 2
	  || GET_MODE_SIZE (GET_MODE (src)) != 2)
	continue;

      while (SUBREG_P (dest))
	dest = SUBREG_REG (dest);
      while (SUBREG_P (src))
	src = SUBREG_REG (src);

      if (! REG_P (dest)
	  || ! REG_P (src)
	  || GET_MODE_SIZE (GET_MODE (dest)) != 2
	  || GET_MODE_SIZE (GET_MODE (src)) != 2)
	continue;

      rd = REGNO (dest);
      rs = REGNO (src);

      if (rd == A_REG)
	{
	  if (! ia16_regno_in_class_p (rs, GENERAL_REGS)
	      || ! find_regno_note (insn, REG_DEAD, rs)
	      || (rs < FIRST_NOQI_REG - 1
		  && ! find_regno_note (insn, REG_DEAD, rs + 1)))
	    continue;
	  PATTERN (insn) = gen__xchghi2 (gen_rtx_REG (HImode, rs),
					 gen_rtx_REG (HImode, A_REG));
	  ia16_recog (insn);
	  reg_now_clobbered_p[rs] = true;
	}
      else if (rs == A_REG)
	{
	  if (! ia16_regno_in_class_p (rd, GENERAL_REGS)
	      || ! find_regno_note (insn, REG_DEAD, A_REG)
	      || ! find_regno_note (insn, REG_DEAD, AH_REG))
	    continue;
	  PATTERN (insn) = gen__xchghi2 (gen_rtx_REG (HImode, rd),
					 gen_rtx_REG (HImode, A_REG));
	  ia16_recog (insn);
	  reg_now_clobbered_p[A_REG] = true;
	}
    }

  /* If e.g. `movw %bx, %ax' was rewritten into `xchgw %bx, %ax', then any
     REG_EQUAL or REG_EQUIV notes involving %bx may no longer be correct.
     Remove such notes.  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (! INSN_P (insn))
	continue;

      pat = single_set (insn);
      if (! pat)
	continue;

      dest = SET_DEST (pat);
      while (SUBREG_P (dest))
	dest = SUBREG_REG (dest);

      if (! REG_P (dest))
	continue;

      rd = REGNO (dest);
      if (rd <= LAST_ALLOCABLE_REG
	  && (reg_now_clobbered_p[rd]
	      || (rd < FIRST_NOQI_REG && reg_now_clobbered_p[rd / 2 * 2])))
	remove_note (insn, find_reg_equal_equiv_note (insn));
    }
}

void
ia16_machine_dependent_reorg (void)
{
  /* Do correctness checks.  */
  ia16_verify_calls_from_no_assume_ds ();
  ia16_verify_calls_from_no_assume_ss ();

  /* Do optimizations.  */
  if (optimize)
    {
      compute_bb_for_insn ();
      ia16_rewrite_reg_parm_save_as_push ();

      if (TARGET_ALLOCABLE_DS_REG && call_used_regs[DS_REG])
	ia16_rewrite_bp_as_bx ();

      /* The insn notes are only needed by the `movw' -> `xchgw' rewriting
	 subpass (ia16_rewrite_movw_as_xchg ()), but I put the df_analyze ()
	 before the %ss elimination subpass (ia16_elide_unneeded_ss_stuff
	 ()).

	 This is because df_analyze () might overwrite the %ss subpass's
	 judgement on whether %ds is live.  (This caused a minor regression
	 in gcc.target/ia16/torture/optimize/opt-1.c, as an extraneous %ss
	 was sticking out.)  -- tkchia  */
      if (optimize_size)
	{
	  df_note_add_problem ();
	  df_analyze ();
	}

      if (TARGET_ALLOCABLE_DS_REG && call_used_regs[DS_REG])
	{
	  if (ia16_in_ss_data_function_p ())
	    {
	      ia16_elide_unneeded_ss_stuff ();
	      ia16_tweak_reset_ds ();
	    }
	}

      free_bb_for_insn ();

      if (optimize_size)
	ia16_rewrite_movw_as_xchgw ();
    }
}
