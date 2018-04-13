/* Subroutines used for macro/preprocessor support for Intel 16-bit x86.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.
   Very preliminary IA-16 far pointer support and __IA16_* macros by TK Chia

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "c-family/c-common.h"
#include "c-family/c-pragma.h"
#include "hard-reg-set.h"

static void
def_or_undef_macro (const char *name, bool def_p)
{
  if (def_p)
    cpp_define (parse_in, name);
  else
    cpp_undef (parse_in, name);
}

/* Implements TARGET_CPU_CPP_BUILTINS.  */
void
ia16_cpu_cpp_builtins (void)
{
  builtin_define_std ("ia16");
  cpp_define (parse_in, "__FAR");

  /* Define macros corresponding to features supported in the chosen -march=
     architecture.  Here I follow the ARM convention of defining macros with
     names __*_FEATURE_*, since it seems clearer than the i386 convention of
     using just the feature names.

     I omit TARGET_TUNE_8BIT --- it does not denote a CPU capability, and
     should probably be handled another way.  -- tkchia  */
  def_or_undef_macro ("__IA16_FEATURE_SHIFT_IMM", TARGET_SHIFT_IMM);
  def_or_undef_macro ("__IA16_FEATURE_PUSH_IMM", TARGET_PUSH_IMM);
  def_or_undef_macro ("__IA16_FEATURE_IMUL_IMM", TARGET_IMUL_IMM);
  def_or_undef_macro ("__IA16_FEATURE_PUSHA", TARGET_PUSHA);
  def_or_undef_macro ("__IA16_FEATURE_ENTER_LEAVE", TARGET_ENTER_LEAVE);
  def_or_undef_macro ("__IA16_FEATURE_SHIFT_MASKED", TARGET_SHIFT_MASKED);
  def_or_undef_macro ("__IA16_FEATURE_AAD_IMM", TARGET_AAD_IMM);
  def_or_undef_macro ("__IA16_FEATURE_FSTSW_AX", TARGET_FSTSW_AX);

  /* If %ds is an allocatable register, define a macro to advertise this.  */
  def_or_undef_macro ("__IA16_FEATURE_ALLOCABLE_DS_REG",
		      TARGET_ALLOCABLE_DS_REG);

  /* Define a macro saying whether we are generating code for protected mode
     operation.  */
  def_or_undef_macro ("__IA16_FEATURE_PROTECTED_MODE", TARGET_PROTECTED_MODE);

  /* Define macros for various compiler features:
       * far static variables
       * far functions of the form `int foo (void) __far;'
       * function calling conventions available through __attribute__ ((.)). 
   */
  def_or_undef_macro ("__IA16_FEATURE_FAR_STATIC_STORAGE",
		      TARGET_CMODEL_IS_SMALL);
  cpp_define (parse_in, "__IA16_FEATURE_FAR_FUNCTION_SYNTAX");
  cpp_define (parse_in, "__IA16_FEATURE_ATTRIBUTE_CDECL");
  cpp_define (parse_in, "__IA16_FEATURE_ATTRIBUTE_STDCALL");

  /* Also define a macro to give the function calling convention settings
     in use.  */
  def_or_undef_macro ("__IA16_CALLCVT_CDECL", ! TARGET_RTD);
  def_or_undef_macro ("__IA16_CALLCVT_STDCALL", TARGET_RTD);
  def_or_undef_macro ("__IA16_CALLCVT_FAR_FUNCTION_IF_FAR_RETURN_TYPE",
    TARGET_FAR_FUNCTION_IF_FAR_RETURN_TYPE);

  /* Define macros corresponding to the chosen memory model.  I define both
     an AArch64-style macro __IA16_CMODEL_{TINY | SMALL}__, and a simple
     __{TINY | SMALL}__ macro as used in the classical Borland C and Open
     Watcom compilers (and others).

     This code currently assumes that there are only two memory models ---
     "tiny" and "small".  If more memory models are added, we will probably
     need to define an `enum' in a whole separate header file, similar to
     gcc/config/i386/i386-opts.h .  -- tkchia */
  def_or_undef_macro ("__IA16_CMODEL_TINY__", ! TARGET_CMODEL_IS_SMALL);
  def_or_undef_macro ("__TINY__", ! TARGET_CMODEL_IS_SMALL);
  def_or_undef_macro ("__IA16_CMODEL_SMALL__", TARGET_CMODEL_IS_SMALL);
  def_or_undef_macro ("__SMALL__", TARGET_CMODEL_IS_SMALL);

  /* Define a macro for the chosen -march=.  A source file can use this to
     decide whether to employ a capability not covered by the
     __IA16_FEATURE_* macros, e.g. the `arpl' or the `bound' instruction.

     Following the practice for both the ARM and i386 ports, only one macro
     is defined.  This means that for some features, a user may need to test
     for several macros --- e.g. `bound' is supported if the compiler defines
     either __IA16_ARCH_ANY_186, __IA16_ARCH_80186, __IA16_ARCH_NEC_V20,
     __IA16_ARCH_NEC_V30, or __IA16_ARCH_I80286.

     This is obviously clunky from the user's viewpoint, but I am not sure
     what a _really_ good alternative might be.  -- tkchia  */
  def_or_undef_macro ("__IA16_ARCH_ANY", target_arch == PROCESSOR_ANY);
  def_or_undef_macro ("__IA16_ARCH_ANY_186", target_arch == PROCESSOR_ANY_186);
  def_or_undef_macro ("__IA16_ARCH_I8086", target_arch == PROCESSOR_I8086);
  def_or_undef_macro ("__IA16_ARCH_I8088", target_arch == PROCESSOR_I8088);
  def_or_undef_macro ("__IA16_ARCH_NEC_V30", target_arch == PROCESSOR_NEC_V30);
  def_or_undef_macro ("__IA16_ARCH_NEC_V20", target_arch == PROCESSOR_NEC_V20);
  def_or_undef_macro ("__IA16_ARCH_I80186", target_arch == PROCESSOR_I80186);
  def_or_undef_macro ("__IA16_ARCH_I80188", target_arch == PROCESSOR_I80188);
  def_or_undef_macro ("__IA16_ARCH_I80286", target_arch == PROCESSOR_I80286);

  /* Define a macro for the chosen -mtune=, for good measure.  */
  def_or_undef_macro ("__IA16_TUNE_I8086", target_tune == PROCESSOR_I8086);
  def_or_undef_macro ("__IA16_TUNE_I8088", target_tune == PROCESSOR_I8088);
  def_or_undef_macro ("__IA16_TUNE_NEC_V30", target_tune == PROCESSOR_NEC_V30);
  def_or_undef_macro ("__IA16_TUNE_NEC_V20", target_tune == PROCESSOR_NEC_V20);
  def_or_undef_macro ("__IA16_TUNE_I80186", target_tune == PROCESSOR_I80186);
  def_or_undef_macro ("__IA16_TUNE_I80188", target_tune == PROCESSOR_I80188);
  def_or_undef_macro ("__IA16_TUNE_I80286", target_tune == PROCESSOR_I80286);
}

/* Implements REGISTER_TARGET_PRAGMAS.  */
void
ia16_register_pragmas (void)
{
  c_register_addr_space ("__far", ADDR_SPACE_FAR);
}
