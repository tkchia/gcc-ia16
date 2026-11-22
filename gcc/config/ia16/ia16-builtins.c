/* Machine-specific built-in functions for Intel 16-bit x86 target.
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
#include "gimple.h"
#include "cfgloop.h"
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
#include "gimple-iterator.h"
#include "tree-vectorizer.h"
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
#include "targhooks.h"

#include "target-def.h"

tree ia16_builtin_decls[IA16_BUILTIN_MAX];
tree ia16_void_far_ptr_type_node = NULL_TREE;

tree
ia16_gimplify_va_arg_expr (tree valist, tree type, gimple_seq *pre_p,
                           gimple_seq *post_p)
{
  tree f_pv, f_pssv, pv, pssv;

  if (TARGET_CMODEL_IS_FAR_DATA)
    ;
  else if (ia16_in_ss_data_function_p ())
    {
      f_pv = TYPE_FIELDS (va_list_type_node);
      pv = build3 (COMPONENT_REF, TREE_TYPE (f_pv), valist, f_pv, NULL_TREE);
      valist = pv;
    }
  else
    {
      f_pv = TYPE_FIELDS (va_list_type_node);
      f_pssv = DECL_CHAIN (f_pv);
      pssv = build3 (COMPONENT_REF, TREE_TYPE (f_pssv), valist, f_pssv,
		     NULL_TREE);
      valist = pssv;
      type
	= build_qualified_type (type,
				TYPE_QUALS_NO_ADDR_SPACE (type)
				| ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_SEG_SS));
    }
  return std_gimplify_va_arg_expr (valist, type, pre_p, post_p);
}

void
ia16_init_builtins (void)
{
  tree intSEG_type_node = unsigned_intHI_type_node,
       const_void_far_type_node, const_void_far_ptr_type_node,
       void_far_type_node,
       intSEG_ftype_intHI, unsigned_intHI_ftype_const_void_far_ptr,
       intSEG_ftype_void, void_far_ptr_ftype_const_void_ptr, func;

  /*
   * With -mprotected-mode, we need a type node for PHImode for use with
   * __builtin_ia16_selector (), but there is no such existing node.  So
   * fashion one.
   */
  if (TARGET_PROTECTED_MODE)
    {
      intSEG_type_node = build_distinct_type_copy (unsigned_intHI_type_node);
      SET_TYPE_MODE (intSEG_type_node, PHImode);
    }
  (*lang_hooks.types.register_builtin_type) (intSEG_type_node, "__segment");
  const_void_far_type_node
    = build_qualified_type (void_type_node,
			    TYPE_QUAL_CONST
			    | ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
  const_void_far_ptr_type_node = build_pointer_type (const_void_far_type_node);
  void_far_type_node
    = build_qualified_type (void_type_node,
			    ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
  ia16_void_far_ptr_type_node = build_pointer_type (void_far_type_node);

  intSEG_ftype_intHI = build_function_type_list (intSEG_type_node,
						 unsigned_intHI_type_node,
						 NULL_TREE);
  unsigned_intHI_ftype_const_void_far_ptr
    = build_function_type_list (unsigned_intHI_type_node,
				const_void_far_ptr_type_node, NULL_TREE);
  intSEG_ftype_void = build_function_type (intSEG_type_node, void_list_node);
  void_far_ptr_ftype_const_void_ptr
    = build_function_type_list (ia16_void_far_ptr_type_node,
				const_ptr_type_node, NULL_TREE);
  func = add_builtin_function ("__builtin_ia16_selector", intSEG_ftype_intHI,
			       IA16_BUILTIN_SELECTOR, BUILT_IN_MD, NULL,
			       NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_SELECTOR] = func;

  func = add_builtin_function ("__builtin_ia16_FP_OFF",
			       unsigned_intHI_ftype_const_void_far_ptr,
			       IA16_BUILTIN_FP_OFF, BUILT_IN_MD, NULL,
			       NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_FP_OFF] = func;

  func = add_builtin_function ("__builtin_ia16_static_far_cast",
			       void_far_ptr_ftype_const_void_ptr,
			       IA16_BUILTIN_STATIC_FAR_CAST, BUILT_IN_MD, NULL,
			       NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_STATIC_FAR_CAST] = func;

  func = add_builtin_function ("__builtin_ia16_near_data_segment",
			       intSEG_ftype_void,
			       IA16_BUILTIN_NEAR_DATA_SEGMENT, BUILT_IN_MD,
			       NULL, NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_NEAR_DATA_SEGMENT] = func;

  func = add_builtin_function ("__builtin_ia16_ss",
			       intSEG_ftype_void, IA16_BUILTIN_SS,
			       BUILT_IN_MD, NULL, NULL_TREE);
  TREE_READONLY (func) = 1;
  ia16_builtin_decls[IA16_BUILTIN_SS] = func;

  /*
   * Minor black magic: if the current memory model uses a near data
   * segment, then tweak the prototypes of __builtin_va_start (AP, ...),
   * __builtin_va_end (AP), & __builtin_va_copy (AP, ), so that they can
   * accept an AP that is outside the generic data segment.
   */
  if (! TARGET_CMODEL_IS_FAR_DATA)
    {
      tree va_list_ref_type_node = build_reference_type (va_list_type_node);
      tree va_list_far_type_node
	     = build_qualified_type (va_list_type_node,
				     ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
      tree va_list_far_ref_type_node
	     = build_reference_type (va_list_far_type_node);

      tree va_start_far_type_node
	= build_varargs_function_type_list (void_type_node,
					    va_list_far_ref_type_node,
					    NULL_TREE);
      tree va_end_far_type_node
	= build_function_type_list (void_type_node, va_list_far_ref_type_node,
				    NULL_TREE);
      tree va_copy_far_type_node
	= build_function_type_list (void_type_node, va_list_far_ref_type_node,
				    va_list_type_node, NULL_TREE);

      tree va_start_func = builtin_decl_explicit (BUILT_IN_VA_START);
      tree va_end_func = builtin_decl_explicit (BUILT_IN_VA_END);
      tree va_copy_func = builtin_decl_explicit (BUILT_IN_VA_COPY);

      TREE_TYPE (va_start_func) = va_start_far_type_node;
      TREE_TYPE (va_end_func) = va_end_far_type_node;
      TREE_TYPE (va_copy_func) = va_copy_far_type_node;
    }
}

tree
ia16_builtin_decl (unsigned code, bool initialize_p ATTRIBUTE_UNUSED)
{
  if (code < IA16_BUILTIN_MAX)
    return ia16_builtin_decls[code];
  else
    return error_mark_node;
}

rtx
ia16_expand_builtin (tree expr, rtx target ATTRIBUTE_UNUSED,
		     rtx subtarget ATTRIBUTE_UNUSED,
		     machine_mode mode ATTRIBUTE_UNUSED,
		     int ignore ATTRIBUTE_UNUSED)
{
  tree fndecl = TREE_OPERAND (CALL_EXPR_FN (expr), 0), arg0;
  rtx op0, res;
  unsigned fcode = DECL_FUNCTION_CODE (fndecl);

  switch (fcode)
    {
    case IA16_BUILTIN_SELECTOR:
      arg0 = CALL_EXPR_ARG (expr, 0);
      op0 = expand_normal (arg0);
      return ia16_bless_selector (op0);

    case IA16_BUILTIN_FP_OFF:
      arg0 = CALL_EXPR_ARG (expr, 0);
      op0 = expand_normal (arg0);
      return ia16_far_pointer_offset (op0);

    case IA16_BUILTIN_NEAR_DATA_SEGMENT:
      res = ia16_data_seg_rtx ();
      if (res)
	return res;
      /* fall through */

    case IA16_BUILTIN_SS:
      return gen_rtx_REG (SEGmode, SS_REG);

    default:
      gcc_unreachable ();
    }
}

static bool
ia16_fp_off_foldable_for_as (addr_space_t as)
{
  return as == ADDR_SPACE_FAR || as == ADDR_SPACE_GENERIC;
}

static bool
ia16_static_far_cast_foldable_for_as (addr_space_t as)
{
  return as == ADDR_SPACE_GENERIC;
}

static tree
ia16_fallback_far_cast (tree op)
{
  return fold_build1 (ADDR_SPACE_CONVERT_EXPR, ia16_void_far_ptr_type_node,
		      op);
}

tree
ia16_resolve_overloaded_builtin (unsigned loc, tree fndecl, void *arglist)
{
  unsigned fcode = DECL_FUNCTION_CODE (fndecl);
  vec<tree, va_gc>& args = * (vec<tree, va_gc> *) arglist;
  tree op, type, deref_type, far_deref_type, far_ptr_type,
       rfndecl, void_far_ptr_expr;

  switch (fcode)
    {
    case IA16_BUILTIN_STATIC_FAR_CAST:
      if (args.length () != 1)
	{
	  error_at (loc, "%qs expects 1 argument but %u given",
			 "__builtin_ia16_static_far_cast",
			 (unsigned) args.length ());
	  return error_mark_node;
	}

      /* 1) If the argument is already a far pointer, just return the pointer.
	 2) If the argument is a near pointer to non-void, call the actual
	    built-in -- which returns a void __far * -- then cast to a far
	    pointer to non-void.
	 3) Otherwise let ia16_fold_builtin (...) below process this built-in
	    call.  */
      op = args[0];
      type = TREE_TYPE (op);

      if (! POINTER_TYPE_P (type))
	return NULL_TREE;  /* ?!? */

      deref_type = TREE_TYPE (type);
      if (TYPE_ADDR_SPACE (deref_type) == ADDR_SPACE_FAR)
	return op;

      if (VOID_TYPE_P (deref_type))
	return NULL_TREE;  /* NOTE: this is case 3! */

      far_deref_type
	= build_qualified_type (deref_type,
				ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
      far_ptr_type = build_pointer_type (far_deref_type);

      rfndecl = targetm.builtin_decl (fcode, true);
      op = fold_build1 (NOP_EXPR, ptr_type_node, op);
      void_far_ptr_expr = build_call_expr (rfndecl, 1, op);
      return fold_build1 (NOP_EXPR, far_ptr_type, void_far_ptr_expr);

    default:
      return NULL_TREE;
    }
}

tree
ia16_fold_builtin (tree fndecl, int n_args, tree *args,
		   bool ignore ATTRIBUTE_UNUSED)
{
  unsigned fcode = DECL_FUNCTION_CODE (fndecl);
  tree op, name, fake, faketype, ptrtype;

  switch (fcode)
    {
    case IA16_BUILTIN_SELECTOR:
      gcc_assert (n_args == 1);
      /* If -mprotected-mode is off, `__builtin_ia16_selector (seg)' is the
	 same as `seg', so we can fold the call right here.  If not, pass
	 the buck to ia16_expand_builtin (...) above.  */
      if (TARGET_PROTECTED_MODE)
	return NULL_TREE;
      return args[0];

    case IA16_BUILTIN_FP_OFF:
      gcc_assert (n_args == 1);
      /* Do a hack to handle the special case of using a far pointer's
	 offset in a static initializer, like so:
		void foo (void) __far;
		static unsigned bar = __builtin_ia16_FP_OFF (foo);
	 The FreeDOS kernel code does something like the above when installing
	 its interrupt handlers.  */
      op = args[0];

      if (! TREE_CONSTANT (op))
	return NULL_TREE;

      /* Check if we are taking the address of a public, external, or static
	 near/far function or near/far variable; if not, again hand over to
	 ia16_expand_builtin (...).  First strip away layers of NOP_EXPR.

	 TODO: generalize this to handle other types of far addresses.  */
      while (TREE_CODE (op) == NOP_EXPR
	     && POINTER_TYPE_P (TREE_TYPE (op))
	     && ia16_fp_off_foldable_for_as (TYPE_ADDR_SPACE
					     (TREE_TYPE (TREE_TYPE (op)))))
	op = TREE_OPERAND (op, 0);

      if (TREE_CODE (op) != ADDR_EXPR
	  || ! POINTER_TYPE_P (TREE_TYPE (op))
	  || ! ia16_fp_off_foldable_for_as (TYPE_ADDR_SPACE
					    (TREE_TYPE (TREE_TYPE (op)))))
	return NULL_TREE;

      op = TREE_OPERAND (op, 0);
      if (! VAR_OR_FUNCTION_DECL_P (op))
	return NULL_TREE;
      if (! DECL_EXTERNAL (op) && ! TREE_PUBLIC (op) && ! TREE_STATIC (op))
	return NULL_TREE;
      if (DECL_EXTERNAL (op) && flag_lto)
	return NULL_TREE;

      /* Mark the original declaration as addressable.  */
      mark_addressable (op);

      /* Create a fake "declaration" which aliases to the same assembler
	 name, but which is placed in the generic address space.  Then take
	 the address of that.  */
      name = DECL_ASSEMBLER_NAME (op);
      faketype = build_qualified_type (TREE_TYPE (op), TYPE_UNQUALIFIED);
      fake = build_decl (UNKNOWN_LOCATION,
			 VAR_P (op) ? VAR_DECL : FUNCTION_DECL,
			 create_tmp_var_name ("__ia16_near_alias"), faketype);
      DECL_ATTRIBUTES (fake) = tree_cons (get_identifier ("weakref"), NULL,
					  DECL_ATTRIBUTES (fake));
      DECL_ATTRIBUTES (fake)
	= tree_cons (get_identifier ("alias"), tree_cons (NULL, name, NULL),
		     DECL_ATTRIBUTES (fake));
      TREE_STATIC (fake) = 1;
      DECL_ARTIFICIAL (fake) = 1;
      TREE_USED (fake) = 1;
      assemble_alias (fake, name);

      ptrtype = build_pointer_type_for_mode (faketype, HImode, true);
      op = build_fold_addr_expr_with_type_loc (UNKNOWN_LOCATION, fake,
					       ptrtype);
      op = fold_build1 (NOP_EXPR, unsigned_intHI_type_node, op);
      return op;

    case IA16_BUILTIN_STATIC_FAR_CAST:
      gcc_assert (n_args == 1);
      /* If the argument is not a constant, then turn this into a normal
	 far cast.  */
      op = args[0];
      if (! TREE_CONSTANT (op))
	return ia16_fallback_far_cast (op);

      /* Check if we are taking the address of a public, external, or static
	 near variable; if not, just do a normal pointer cast.  First strip
	 away layers of NOP_EXPR.  */
      while (TREE_CODE (op) == NOP_EXPR
	     && POINTER_TYPE_P (TREE_TYPE (op))
	     && ia16_static_far_cast_foldable_for_as
		  (TYPE_ADDR_SPACE (TREE_TYPE (TREE_TYPE (op)))))
	op = TREE_OPERAND (op, 0);

      if (TREE_CODE (op) != ADDR_EXPR
	  || ! POINTER_TYPE_P (TREE_TYPE (op))
	  || ! ia16_static_far_cast_foldable_for_as
		 (TYPE_ADDR_SPACE (TREE_TYPE (TREE_TYPE (op)))))
	return ia16_fallback_far_cast (op);

      op = TREE_OPERAND (op, 0);
      if (! VAR_P (op))
	return ia16_fallback_far_cast (op);
      if (! DECL_EXTERNAL (op) && ! TREE_PUBLIC (op) && ! TREE_STATIC (op))
	return ia16_fallback_far_cast (op);
      if (DECL_EXTERNAL (op) && flag_lto)
	return ia16_fallback_far_cast (op);

      /* Mark the original declaration as addressable.  */
      mark_addressable (op);

      /* Create a fake "declaration" which aliases to the same assembler
	 name, but which is placed in the far address space.  Then take the
	 address of that.  */
      name = DECL_ASSEMBLER_NAME (op);
      faketype = build_qualified_type
		   (TREE_TYPE (op), ENCODE_QUAL_ADDR_SPACE (ADDR_SPACE_FAR));
      fake = build_decl (UNKNOWN_LOCATION,
			 VAR_P (op) ? VAR_DECL : FUNCTION_DECL,
			 create_tmp_var_name ("__ia16_far_alias"), faketype);
      DECL_ATTRIBUTES (fake) = tree_cons (get_identifier ("weakref"), NULL,
					  DECL_ATTRIBUTES (fake));
      DECL_ATTRIBUTES (fake)
	= tree_cons (get_identifier ("alias"), tree_cons (NULL, name, NULL),
		     DECL_ATTRIBUTES (fake));
      TREE_STATIC (fake) = 1;
      DECL_ARTIFICIAL (fake) = 1;
      TREE_USED (fake) = 1;
      assemble_alias (fake, name);

      ptrtype = build_pointer_type_for_mode (faketype, SImode, true);
      op = build_fold_addr_expr_with_type_loc (UNKNOWN_LOCATION, fake,
					       ptrtype);
      op = fold_build1 (NOP_EXPR, ia16_void_far_ptr_type_node, op);
      return op;

    case IA16_BUILTIN_NEAR_DATA_SEGMENT:
    case IA16_BUILTIN_SS:
      return NULL_TREE;

    default:
      gcc_unreachable ();
    }
}
