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

static GTY (()) struct target_rtl *ss_data_target_rtl = NULL;
static GTY (()) struct target_rtl *no_ss_data_target_rtl = NULL;

/* Given a type TYPE, return a version of the type which is in the __seg_ss
   address space (unless TYPE is already in a non-generic address space).  */
static tree
ia16_stackify_type (tree type)
{
  int quals;

  if (! ADDR_SPACE_GENERIC_P (TYPE_ADDR_SPACE (type)))
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
ia16_insert_attributes (tree node, tree *attr_ptr ATTRIBUTE_UNUSED)
{
  if (TARGET_ASSUME_SS_DATA)
    return;

  switch (TREE_CODE (node))
    {
    case PARM_DECL:
      DECL_ARG_TYPE (node) = ia16_stackify_type (DECL_ARG_TYPE (node));
      TREE_TYPE (node) = ia16_stackify_type (TREE_TYPE (node));
      break;

    case VAR_DECL:
      if (! TREE_STATIC (node) && ! TREE_PUBLIC (node)
	  && ! DECL_EXTERNAL (node))
	TREE_TYPE (node) = ia16_stackify_type (TREE_TYPE (node));
      break;

    default:
      break;
    }
}

void
ia16_set_current_function (tree decl)
{
  tree type = decl ? TREE_TYPE (decl) : NULL_TREE;
  bool need_no_ss_data = ! ia16_ss_data_function_type_p (type);
  struct target_rtl *r;

  if (! ss_data_target_rtl)
    ss_data_target_rtl = this_target_rtl;

  if (! need_no_ss_data)
    r = ss_data_target_rtl;
  else if (no_ss_data_target_rtl)
    r = no_ss_data_target_rtl;
  else
    {
      int i;

      r = ggc_alloc <target_rtl> ();
      *r = *ss_data_target_rtl;

      for (i = 0; i < (int) MAX_MACHINE_MODE; ++i)
	{
	  mem_attrs *seg_ss_attrs = ggc_alloc <mem_attrs> ();
	  *seg_ss_attrs = *ss_data_target_rtl->x_mode_mem_attrs[i];
	  seg_ss_attrs->addrspace = ADDR_SPACE_SEG_SS;
	  r->x_mode_mem_attrs[i] = seg_ss_attrs;
	}

      no_ss_data_target_rtl = r;
    }

  this_target_rtl = r;
}
