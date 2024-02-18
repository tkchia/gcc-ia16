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
#include "output.h"

static void
def_macro (const char *name)
{
  cpp_define (parse_in, name);
}

static void
def_or_undef_macro (const char *name, bool def_p)
{
  if (def_p)
    def_macro (name);
  else
    cpp_undef (parse_in, name);
}

/* Whether to support the __far & __seg_ss address space qualifiers.

   As of writing, these qualifiers are really only recognized when
   processing the C language specifically, not Objective-C or C++, & only if
   GNU language extensions are enabled.  -- tkchia 20201002 */
static bool
have_addr_spaces_p (void)
{
  return ! c_dialect_cxx () && ! c_dialect_objc () && ! flag_no_asm;
}

/* Implements TARGET_CPU_CPP_BUILTINS.  */
void
ia16_cpu_cpp_builtins (void)
{
  bool def_p;
  char *defn;
  int rv;

  def_macro ("__ia16__=20240218L");

  if (have_addr_spaces_p ())
    {
      def_macro ("__FAR");
      def_macro ("__SEG_SS");
    }

  /* These are for compatibility with programs that check for Open Watcom
     macros.  __X86__ & _M_IX86 indicate an Intel x86 target, while the
     other macros specifically indicate a 16-bit Intel x86 target.

     On _M_IX86, the _Open Watcom C/C++ User's Guide_ says,
	"The _M_IX86 macro is identically equal to 100 times the
	 architecture compiler option value (-0, -1, -2, -3, -4, -5, etc.)."

     (Open Watcom actually also defines M_I86, but this intrudes into the
     user namespace.)  -- tkchia */
  def_macro ("__X86__");
  def_macro ("__I86__");
  def_macro ("_M_I86");
  switch (target_arch)
    {
    default:
      def_macro ("_M_IX86=0");
      break;
    case PROCESSOR_ANY_186:
    case PROCESSOR_NEC_V30:
    case PROCESSOR_NEC_V30MZ:
    case PROCESSOR_NEC_V20:
    case PROCESSOR_I80186:
    case PROCESSOR_I80188:
      def_macro ("_M_IX86=100");
      break;
    case PROCESSOR_I80286:
      def_macro ("_M_IX86=200");
    }

  /* The Amsterdam Compiler Kit defines this. */
  def_macro ("__i86");

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

  /* Define macros saying whether we are generating code for protected mode
     operation, and for DPMI-able operation.  */
  def_or_undef_macro ("__IA16_FEATURE_PROTECTED_MODE", TARGET_PROTECTED_MODE);
  def_or_undef_macro ("__IA16_FEATURE_DOSX", TARGET_DOSX || TARGET_DOSX32);
  def_or_undef_macro ("__IA16_FEATURE_DPMIABLE", TARGET_DOSX || TARGET_DOSX32);
  def_or_undef_macro ("__IA16_FEATURE_DOSX32", TARGET_DOSX32);

  /* Define macros for various compiler features:
       * far static variables
       * far functions of the form `int foo (void) __far;'
       * function calling conventions available through __attribute__ ((.))
       * calling near functions from __attribute__ ((far_section)) functions
       * __builtin_ia16_... (.) functions
       * etc.  */
  def_or_undef_macro ("__IA16_FEATURE_FAR_STATIC_STORAGE",
		      ! TARGET_CMODEL_IS_TINY);
  def_macro ("__IA16_FEATURE_FAR_FUNCTION_SYNTAX");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_CDECL");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_STDCALL");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_REGPARMCALL");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_PASCAL");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_ASSUME_DS_DATA");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_NO_ASSUME_DS_DATA");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_SAVE_DS");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_NO_SAVE_DS");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_SAVE_ES");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_SAVE_ALL");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_ASSUME_SS_DATA");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_NO_ASSUME_SS_DATA");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_NEAR_SECTION");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_FAR_SECTION");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_INTERRUPT");
  def_macro ("__IA16_FEATURE_ATTRIBUTE_AUTOFLOAT_STDIO_V2");
  def_macro ("__IA16_FEATURE_FAR_SECTION_TO_NEAR_CALLS");
  def_or_undef_macro ("__IA16_FEATURE_SEGMENT_RELOCATION_STUFF",
		      TARGET_SEG_RELOC_STUFF);
  def_or_undef_macro ("__IA16_FEATURE_NEWLIB_AUTOFLOAT_STDIO",
		      TARGET_NEWLIB_AUTOFLOAT_STDIO);
  def_or_undef_macro ("__IA16_FEATURE_TSR", TARGET_TSR);
  def_macro ("__BUILTIN_IA16_SELECTOR");
  def_macro ("__BUILTIN_IA16_FP_OFF");
  def_macro ("__BUILTIN_IA16_STATIC_FAR_CAST");
  def_macro ("__BUILTIN_IA16_NEAR_DATA_SEGMENT");
  def_macro ("__BUILTIN_IA16_SS");

  /* Also define macros to give the function calling convention settings
     and/or the object file ABI variant in use (segelf vs. non-segelf),
     as specified on the command line.  */
  def_or_undef_macro ("__IA16_CALLCVT_CDECL",
		      target_call_parm_cvt == CALL_PARM_CVT_CDECL);
  def_or_undef_macro ("__IA16_CALLCVT_STDCALL",
		      target_call_parm_cvt == CALL_PARM_CVT_STDCALL);
  def_or_undef_macro ("__IA16_CALLCVT_REGPARMCALL",
		      target_call_parm_cvt == CALL_PARM_CVT_REGPARMCALL);
  def_or_undef_macro ("__IA16_CALLCVT_PASCAL", /* unlikely, but just in case */
		      target_call_parm_cvt == CALL_PARM_CVT_PASCAL);
  def_macro ("__IA16_CALLCVT_FAR_FUNCTION_IF_FAR_RETURN_TYPE");
  def_or_undef_macro ("__IA16_CALLCVT_ASSUME_DS_DATA", TARGET_ASSUME_DS_DATA);
  def_or_undef_macro ("__IA16_CALLCVT_NO_ASSUME_DS_DATA",
		      ! TARGET_ASSUME_DS_DATA);
  def_or_undef_macro ("__IA16_CALLCVT_ASSUME_SS_DATA", TARGET_ASSUME_SS_DATA);
  def_or_undef_macro ("__IA16_CALLCVT_NO_ASSUME_SS_DATA",
		      ! TARGET_ASSUME_SS_DATA);
  def_or_undef_macro ("__IA16_ABI_SEGELF", TARGET_ABI_SEGELF);

  /* Since the `regparmcall' convention is still in a state of flux, I am
     adding a crude versioning mechanism so that users can ask the compiler
     which convention it is actually implementing...  */
  def_macro ("__IA16_REGPARMCALL_ABI=20180814L");

  /* Define macros corresponding to the chosen memory model.  I define both
     an AArch64-style macro __IA16_CMODEL_{TINY | SMALL | ...}__, and a
     simple __{TINY | SMALL | ...}__ macro as used in the classical Borland
     C and Open Watcom compilers (and others).

     Open Watcom and Digital Mars also define _M_I86...M macros, so do that
     here too.  -- tkchia */
  def_p = (target_cmodel == CMODEL_TINY);
  def_or_undef_macro ("__IA16_CMODEL_TINY__", def_p);
  def_or_undef_macro ("__TINY__", def_p);
  def_or_undef_macro ("_M_I86TM", def_p);
  def_p = (target_cmodel == CMODEL_SMALL);
  def_or_undef_macro ("__IA16_CMODEL_SMALL__", def_p);
  def_or_undef_macro ("__SMALL__", def_p);
  def_or_undef_macro ("_M_I86SM", def_p);
  def_p = (target_cmodel == CMODEL_MEDIUM);
  def_or_undef_macro ("__IA16_CMODEL_MEDIUM__", def_p);
  def_or_undef_macro ("__MEDIUM__", def_p);
  def_or_undef_macro ("_M_I86MM", def_p);
  def_p = (target_cmodel == CMODEL_COMPACT);
  def_or_undef_macro ("__IA16_CMODEL_COMPACT__", def_p);
  def_or_undef_macro ("__COMPACT__", def_p);
  def_or_undef_macro ("_M_I86CM", def_p);
  def_p = (target_cmodel == CMODEL_LARGE);
  def_or_undef_macro ("__IA16_CMODEL_LARGE__", def_p);
  def_or_undef_macro ("__LARGE__", def_p);
  def_or_undef_macro ("_M_I86LM", def_p);
  def_p = (target_cmodel == CMODEL_HUGE);
  def_or_undef_macro ("__IA16_CMODEL_HUGE__", def_p);
  def_or_undef_macro ("__HUGE__", def_p);
  def_or_undef_macro ("_M_I86HM", def_p);

  /* Convenience macro.  */
  def_or_undef_macro ("__IA16_CMODEL_IS_FAR_TEXT", TARGET_CMODEL_IS_FAR_TEXT);

  /* Define a macro for the chosen -march=.  A source file can use this to
     decide whether to employ a capability not covered by the
     __IA16_FEATURE_* macros, e.g. the `arpl' or the `bound' instruction.

     Following the practice for both the ARM and i386 ports, only one macro
     is defined.  This means that for some features, a user may need to test
     for several macros --- e.g. `bound' is supported if the compiler defines
     either __IA16_ARCH_ANY_186, __IA16_ARCH_80186, __IA16_ARCH_NEC_V20,
     __IA16_ARCH_NEC_V30, __IA16_ARCH_NEC_V30MZ, or __IA16_ARCH_I80286.

     This is obviously clunky from the user's viewpoint, but I am not sure
     what a _really_ good alternative might be.  -- tkchia  */
  def_or_undef_macro ("__IA16_ARCH_ANY", target_arch == PROCESSOR_ANY);
  def_or_undef_macro ("__IA16_ARCH_ANY_186", target_arch == PROCESSOR_ANY_186);
  def_or_undef_macro ("__IA16_ARCH_I8086", target_arch == PROCESSOR_I8086);
  def_or_undef_macro ("__IA16_ARCH_I8088", target_arch == PROCESSOR_I8088);
  def_or_undef_macro ("__IA16_ARCH_NEC_V30", target_arch == PROCESSOR_NEC_V30);
  def_or_undef_macro ("__IA16_ARCH_NEC_V30MZ", target_arch == PROCESSOR_NEC_V30MZ);
  def_or_undef_macro ("__IA16_ARCH_NEC_V20", target_arch == PROCESSOR_NEC_V20);
  def_or_undef_macro ("__IA16_ARCH_I80186", target_arch == PROCESSOR_I80186);
  def_or_undef_macro ("__IA16_ARCH_I80188", target_arch == PROCESSOR_I80188);
  def_or_undef_macro ("__IA16_ARCH_I80286", target_arch == PROCESSOR_I80286);

  /* Define a macro for the chosen -mtune=, for good measure.  */
  def_or_undef_macro ("__IA16_TUNE_ANY", target_tune == PROCESSOR_ANY);
  def_or_undef_macro ("__IA16_TUNE_ANY_186", target_tune == PROCESSOR_ANY_186);
  def_or_undef_macro ("__IA16_TUNE_I8086", target_tune == PROCESSOR_I8086);
  def_or_undef_macro ("__IA16_TUNE_I8088", target_tune == PROCESSOR_I8088);
  def_or_undef_macro ("__IA16_TUNE_NEC_V30", target_tune == PROCESSOR_NEC_V30);
  def_or_undef_macro ("__IA16_TUNE_NEC_V30MZ", target_tune == PROCESSOR_NEC_V30MZ);
  def_or_undef_macro ("__IA16_TUNE_NEC_V20", target_tune == PROCESSOR_NEC_V20);
  def_or_undef_macro ("__IA16_TUNE_I80186", target_tune == PROCESSOR_I80186);
  def_or_undef_macro ("__IA16_TUNE_I80188", target_tune == PROCESSOR_I80188);
  def_or_undef_macro ("__IA16_TUNE_I80286", target_tune == PROCESSOR_I80286);

  /* Define a macro for the chosen assembly language syntax (-masm=).  */
  def_or_undef_macro ("__IA16_ASM_ATT", ia16_asm_dialect == ASM_ATT);
  def_or_undef_macro ("__IA16_ASM_INTEL", ia16_asm_dialect == ASM_INTEL);

  /* Define predicates for the target machine. */
  cpp_assert (parse_in, "cpu=ia16");
  cpp_assert (parse_in, "cpu=i86");
  cpp_assert (parse_in, "machine=ia16");
  cpp_assert (parse_in, "machine=i86");

  /* For further Open Watcom compatibility, map the words __interrupt,
     __saveregs, etc. to their corresponding GCC attributes.

     While at it, also define macros __INTERRUPT, __SAVEREGS, etc. to
     advertise which of the (lowercase) words we support.

     Note that apparently Open Watcom & Borland C++ have different ideas on
     what the word __saveregs means.  _The Borland C++ 4.0 Programmers
     Guide_ (https://archive.org/details/bitsavers_borlandborn4.0Programmers
     GuideOct93_13237599/) says,
	"The __saveregs modifier causes the function to preserve all register
	 values and restore them before returning (except for explicit return
	 values passed in registers such as AX or DX)."
     As for OW, Jiří Malák says (https://github.com/open-watcom/open-watcom-
     v2/issues/667),
	"__saveregs means to save all segment registers."
     Here GCC follows Borland's stricter meaning.  */
  def_macro ("__cdecl=__attribute__ ((__cdecl__))");
  def_macro ("__stdcall=__attribute__ ((__stdcall__))");
  def_macro ("__pascal=__attribute__ ((__pascal__))");
  def_macro ("__interrupt=__attribute__ ((__interrupt__))");
  def_macro ("__loadds=__attribute__ ((__no_assume_ds_data__))");
  def_macro ("__saveregs=__attribute__ ((__save_all__))");
  def_macro ("__CDECL");
  def_macro ("__STDCALL");
  def_macro ("__PASCAL");
  def_macro ("__INTERRUPT");
  def_macro ("__LOADDS");
  def_macro ("__SAVEREGS");

  /* Also advertise that we support the __segment type name.  */
  def_macro ("__SEGMENT");

  /* Open Watcom defines _STDCALL_SUPPORTED "for compatibility with
     Microsoft". */
  def_macro ("_STDCALL_SUPPORTED");

  /* If the user says -mnewlib-nano-stdio, try to pretend that <newlib.h>
     defines _NANO_FORMATTED_IO (actually this just defines the macro whether
     or not <newlib.h> is used).

     Do not undefine _NANO_FORMATTED_IO if it is explicitly defined.  */
  if (TARGET_NEWLIB_NANO_STDIO)
    def_macro ("_NANO_FORMATTED_IO");

  /* This is used by the ia16.h portion of the machine description to
     implement CRT_CALL_STATIC_FUNCTION (, ).  */
  if (! TARGET_CMODEL_IS_FAR_TEXT)
    rv = asprintf (&defn, "__IA16___CRT_CALL_STATIC_FUNCTION(o,f)"
			  "=__asm(o \"; call %s\" #f \"; .text\");",
		   user_label_prefix);
  else if (! TARGET_ABI_SEGELF)
    rv = asprintf (&defn, "__IA16___CRT_CALL_STATIC_FUNCTION(o,f)"
			  "=__asm(\".pushsection .text; "
				   "\" o \"; "
				   ".reloc .+3, R_386_OZSEG16, %s\" #f \"; "
				   "lcall $0, $%s\" #f \"; "
				   ".text; "
				   ".popsection\");",
		   user_label_prefix, user_label_prefix);
  else
    rv = asprintf (&defn, "__IA16___CRT_CALL_STATIC_FUNCTION(o,f)"
			  "=__asm(\".pushsection .text; "
				   "\" o \"; "
				   ".set __ia16_crt_call, .+3; "
				   ".reloc __ia16_crt_call, R_386_SEG16, "
					  "\\\"%s\" #f \"!\\\"; "
				   "lcall $0, $%s\" #f \"; "
				   ".text; "
				   ".popsection\");",
		   user_label_prefix, user_label_prefix);

  if (rv > 0)
    {
      def_macro (defn);
      free (defn);
    }
}

/* Implements REGISTER_TARGET_PRAGMAS.  */
void
ia16_register_pragmas (void)
{
  if (have_addr_spaces_p ())
    {
      c_register_addr_space ("__far", ADDR_SPACE_FAR);
      c_register_addr_space ("__seg_ss", ADDR_SPACE_SEG_SS);
    }
}
