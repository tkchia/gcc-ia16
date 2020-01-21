/* Hacks for %ss != .data calling conventions for Intel 16-bit x86 target.
   Copyright (C) 2020 Free Software Foundation, Inc.
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

/* XXX: deep black magic ahead.  */

/* These pointers may be NULL-ified by the GCC garbage collector.  */
struct target_rtl *ia16_ss_data_target_rtl = NULL,
		  *ia16_no_ss_data_target_rtl = NULL;

/* GCC invokes TARGET_LRA_P () some time after producing RTL, and just
   before doing register allocation.

   1) For %ss != .data, this is a good time to tag segment override terms ---
      of the form `(unspec:HI [<data-seg-rtx>] UNSPEC_SEG_OVERRIDE)' ---
      segment override terms onto `(mem ...)' addresses that go to the generic
      address space.

      (Exceptions are `(call (mem ...) ...)`, where the `(mem ...)' refer to
      text segment addresses.)

      To make sure that the <data-seg-rtx> is not eliminated or moved around
      too much early on during RTL optimization (!), _and_ ensure that %ds
      is properly restored on function exit:

      a) I add a special-case hack to the "mov<mode>" insn pattern in ia16.md
	 to `(use)' the <data-seg-rtx> whenever it is `(set)' (near the
	 function entry); and

      b) I add insns
		(set (reg:SEG DS_REG) <data-seg-rtx>)
		(use (reg:SEG DS_REG))
	 at every place where the function may exit.

   2) Also take this chance to look for any `(mem/u ...)' RTXs --- which
      should refer to static constant memory --- that erroneously point into
      the __seg_ss address space, and correct them to point into the generic
      address space.

      (FIXME: this latter correction should probably be done at an earlier
      stage.  At least, it should happen before any RTL-level
      optimizations.)  */
static rtx
ia16_rectify_mems (rtx x, enum rtx_code outer_code)
{
  const char *fmt;
  int fmt_len, i;
  enum rtx_code code;
  bool copied = false;
  rtx subx, new_subx;

  if (! x)
    return x;

  code = GET_CODE (x);

  if (code == MEM)
    {
      if (MEM_ADDR_SPACE (x) == ADDR_SPACE_SEG_SS && MEM_READONLY_P (x))
	{
	  x = shallow_copy_rtx (x);
	  set_mem_addr_space (x, ADDR_SPACE_GENERIC);
	  copied = true;
	}

      if (ADDR_SPACE_GENERIC_P (MEM_ADDR_SPACE (x)) && outer_code != CALL)
	{
	  rtx addr = XEXP (x, 0);
	  if (! ia16_have_seg_override_p (addr))
	    {
	      rtx data_seg = ia16_data_seg_rtx (), ovr;
	      gcc_assert (data_seg);
	      ovr = ia16_seg_override_term (data_seg);
	      addr = gen_rtx_PLUS (HImode, addr, ovr);
	      if (! copied)
		{
		  x = shallow_copy_rtx (x);
		  copied = true;
		}
		XEXP (x, 0) = addr;
	    }
	}
    }

  fmt = GET_RTX_FORMAT (code);
  fmt_len = GET_RTX_LENGTH (code);

  for (i = 0; i < fmt_len; ++i)
    {
      switch (fmt[i])
	{
	case 'e':
	  subx = XEXP (x, i);
	  new_subx = ia16_rectify_mems (subx, code);
	  if (new_subx != subx)
	    {
	      if (! copied)
		{
		  x = shallow_copy_rtx (x);
		  copied = true;
		}
	      XEXP (x, i) = new_subx;
	    }
	  break;

	case 'E':
	  if (XVEC (x, i))
	    {
	      bool vec_copied = false;
	      int vec_len = XVECLEN (x, i);
	      int j;

	      for (j = 0; j < vec_len; ++j)
		{
		  subx = XVECEXP (x, i, j);
		  new_subx = ia16_rectify_mems (subx, code);
		  if (new_subx != subx)
		    {
		      if (! copied)
			{
			  x = shallow_copy_rtx (x);
			  copied = true;
			}
		      if (! vec_copied)
			{
			  XVEC (x, i) = shallow_copy_rtvec (XVEC (x, i));
			  vec_copied = true;
			}
		      XVECEXP (x, i, j) = new_subx;
		    }
		}
	    }
	  break;

	default:
	  break;
	}
    }

  return x;
}

static void
add_ds_resets (void)
{
  rtx data_seg = ia16_data_seg_rtx ();
  rtx ds_reg = gen_rtx_REG (SEGmode, DS_REG);
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, EXIT_BLOCK_PTR_FOR_FN (cfun)->preds)
    {
      basic_block bb = e->src;
      rtx_insn *insn = BB_END (bb),
	       *set_insn = make_insn_raw (gen_rtx_SET (ds_reg, data_seg)),
	       *use_insn = make_insn_raw (gen_rtx_USE (SEGmode, ds_reg));

      while (insn != BB_HEAD (bb) && ! INSN_P (insn))
	insn = PREV_INSN (insn);

      if (INSN_P (insn)
	  && (JUMP_P (insn)
	      || (CALL_P (insn) && SIBLING_CALL_P (insn))))
	{
	  add_insn_before (set_insn, insn, bb);
	  add_insn_before (use_insn, insn, bb);
	}
      else
	{
	  add_insn_after (set_insn, insn, bb);
	  add_insn_after (use_insn, set_insn, bb);
	}

      df_insn_rescan (set_insn);
      df_insn_rescan (use_insn);
    }
}

bool
ia16_lra_p (void)
{
  rtx_insn *insn;

  if (! ia16_in_ss_data_function_p ())
    {
      for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
	{
	  if (INSN_P (insn))
	    {
	      rtx pat = PATTERN (insn);
	      rtx new_pat = ia16_rectify_mems (pat, GET_CODE (insn));
	      if (new_pat != pat)
		{
		  PATTERN (insn) = new_pat;
		  ia16_recog (insn);
		  df_insn_rescan (insn);
		}
	    }
	}

      add_ds_resets ();
    }

  return true;
}

/* Given a type TYPE, return a version of the type which is in the __seg_ss
   address space (unless TYPE is already in a non-generic address space, or
   is void).  */
static tree
ia16_stackify_type (tree type)
{
  int quals;

  if (! ADDR_SPACE_GENERIC_P (TYPE_ADDR_SPACE (type))
      || VOID_TYPE_P (type))
    return type;

  quals = TYPE_QUALS_NO_ADDR_SPACE (type)
	  | ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_SEG_SS);

  if (TREE_CODE (type) == ARRAY_TYPE)
    {
      tree new_elem_type = ia16_stackify_type (TREE_TYPE (type));
      type = build_array_type (new_elem_type, TYPE_DOMAIN (type));
    }

  return build_qualified_type (type, quals);
}

void
ia16_override_abi_format (tree fndecl)
{
  tree parm;

  if (! fndecl)
    {
#if 0
      /* This often results in spurious warnings in cases where no actual
	 function calls appear.  So just do nothing here, and rely on the
	 other checking mechanisms in the back-end.  */
      if (! TARGET_ASSUME_SS_DATA)
	warning (OPT_Wmaybe_uninitialized, "unsupported: calling "
		 "compiler support functions with %%ss != .data");
#endif
      return;
    }

  if (ia16_ss_data_function_type_p (TREE_TYPE (fndecl)))
    return;

  /* Make stack parameter declarations refer to the __seg_ss address space. */
  for (parm = DECL_ARGUMENTS (fndecl); parm; parm = DECL_CHAIN (parm))
    {
      TREE_TYPE (parm) = ia16_stackify_type (TREE_TYPE (parm));
      DECL_ARG_TYPE (parm) = ia16_stackify_type (DECL_ARG_TYPE (parm));
    }
}

void
ia16_insert_attributes (tree node, tree *attr_ptr ATTRIBUTE_UNUSED)
{
  /* For every non-static variable declaration in an __attribute__
     ((no_assume_ss_data)) function, make it refer to the __seg_ss address
     space rather than the generic space.

     Stack spill slots etc. are handled by TARGET_SET_CURRENT_FUNCTION.  */
  switch (TREE_CODE (node))
    {
    case PARM_DECL:

    case VAR_DECL:
      if (! ia16_in_ss_data_function_p ()
	  && ! TREE_STATIC (node) && ! TREE_PUBLIC (node)
	  && ! DECL_EXTERNAL (node))
	TREE_TYPE (node) = ia16_stackify_type (TREE_TYPE (node));
      break;

    default:
      break;
    }
}

/* Return a pointer to a `struct target_rtl' with its mode_mem_attrs[]
   structures specifying the address space AS.

   For __attribute__ ((no_assume_ss_data)) functions, this fashions and uses
   mode_mem_attrs[] structures which specify __seg_ss.  This causes
   gen_rtx_MEM (, ) to return (mem ...) RTXs that refer to the __seg_ss
   space on default.

   Yep, this works, except when it does not.  */
static struct target_rtl *
ia16_target_rtl_as (addr_space_t as)
{
  int i;
  struct target_rtl *r;

  if (this_target_rtl->x_mode_mem_attrs[(int) VOIDmode]->addrspace == as)
    return this_target_rtl;

  r = ggc_alloc <target_rtl> ();
  *r = *this_target_rtl;

  for (i = 0; i < (int) MAX_MACHINE_MODE; ++i)
    {
      mem_attrs *as_attrs = ggc_alloc <mem_attrs> ();
      *as_attrs = *this_target_rtl->x_mode_mem_attrs[i];
      as_attrs->addrspace = as;
      r->x_mode_mem_attrs[i] = as_attrs;
    }

  return r;
}

void
ia16_set_current_function (tree decl)
{
  tree type = decl ? TREE_TYPE (decl) : NULL_TREE;
  bool need_no_ss_data = ! ia16_ss_data_function_type_p (type);

  if (! need_no_ss_data)
    {
      if (! ia16_ss_data_target_rtl)
	ia16_ss_data_target_rtl = ia16_target_rtl_as (ADDR_SPACE_GENERIC);
      this_target_rtl = ia16_ss_data_target_rtl;
    }
  else
    {
      if (! ia16_no_ss_data_target_rtl)
	ia16_no_ss_data_target_rtl = ia16_target_rtl_as (ADDR_SPACE_SEG_SS);
      this_target_rtl = ia16_no_ss_data_target_rtl;
    }
}
