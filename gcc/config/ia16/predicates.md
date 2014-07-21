;  Predicate definitions for Intel 16-bit x86.
;  Copyright (C) 2006, 2007 Free Software Foundation, Inc.
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

; Allow anything else than a register_operand.
(define_predicate "nonregister_operand"
   (ior (match_operand 0 "immediate_operand")
	(match_operand 0 "memory_operand"))
)

; This is like "register_operand", but subregs of multiword registers are not
; allowed. This is to prevent the combiner from re-inserting such subregs
; into the insn.
(define_predicate "single_register_operand"
   (and (match_operand 0 "register_operand")
	(ior (not (match_code "subreg"))
	     (match_test "GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) <= UNITS_PER_WORD")))
)

; This is like "nonmemory_operand", but subregs of multiword registers are
; not allowed. This is to prevent the combiner from re-inserting such subregs
; into the insn.
(define_predicate "single_nonmemory_operand"
   (and (match_operand 0 "nonmemory_operand")
	(ior (not (match_code "subreg"))
	     (match_test "GET_MODE_SIZE (GET_MODE (SUBREG_REG (op))) <= UNITS_PER_WORD")))
)

; Like "nonimmediate_operand" except that 0 is allowed.
; This needs to be a special predicate to match const0_rtx.
(define_special_predicate "nonimmediate_or_0_operand"
   (ior (match_operand 0 "nonimmediate_operand")
	(and (match_code "const_int,const_vector")
	     (match_test "op == CONST0_RTX (mode)")))
)

(define_predicate "segment_register_operand"
   (and (match_code "reg")
	(match_test "REGNO (op) == ES_REG"))
)

; Match the memory operand of an xlat instruction (to be split before reload).
; Combine creates this in a number of ways.
; TODO: Make ia16_xlat_cost() use this predicate to reduce code duplication.
(define_predicate "xlat_memory_operand"
; They all have the form (mem:QI (plus:HI (...) (reg/const:HI))).
  (and
    (match_code "mem")
    (match_code "plus" "0")
    (match_code "reg,const_int,symbol_ref,label_ref" "01")
    (ior
; Match (zero_extend:HI (reg/mem/const:QI)).
      (and
	(match_code "zero_extend" "00")
	(match_code "reg,subreg,mem,const_int,symbol_ref,label_ref" "000")
	(match_test "GET_MODE (XEXP (XEXP (XEXP (op, 0), 0), 0)) == QImode")
      )
; Match (zero_extract:HI (reg/mem/const) (const_int 8) (const_int n*8))
; This needs a patch to simplify_rtx().
      (and
	(match_code "zero_extract" "00")
	(match_code "reg,subreg,mem,const_int,symbol_ref,label_ref" "000")
	(match_code "const_int" "001")
	(match_code "const_int" "002")
	(match_test "INTVAL (XEXP (XEXP (XEXP (op, 0), 0), 1)) == 8")
	(match_test "INTVAL (XEXP (XEXP (XEXP (op, 0), 0), 2)) % 8 == 0")
      )
; Match (subreg:HI (zero_extract (reg/mem/const) (const_int 8) (const_int n*8))
;	 0)
      (and
	(match_code "subreg" "00")
	(match_code "zero_extract" "000")
	(match_code "reg,subreg,mem,const_int,symbol_ref,label_ref" "0000")
	(match_code "const_int" "0001")
	(match_code "const_int" "0002")
	(match_test "INTVAL (XEXP (XEXP (XEXP (XEXP (op, 0), 0), 0), 1)) == 8")
	(match_test "INTVAL (XEXP (XEXP (XEXP (XEXP (op, 0), 0), 0), 2)) % 8 == 0")
	(match_test "GET_MODE (XEXP (XEXP (XEXP (op, 0), 0), 0)) != QImode")
      )
; Match (and:HI (subreg (lshiftrt (reg/mem/const) (const_int n*8)) 0)
;		(const_int 255))
      (and
	(match_code "and" "00")
	(match_code "subreg" "000")
	(match_code "const_int" "001")
	(match_code "lshiftrt" "0000")
	(match_code "reg,subreg,mem,const_int,symbol_ref,label_ref" "00000")
	(match_code "const_int" "00001")
	(match_test "INTVAL (XEXP (XEXP (XEXP (op, 0), 0), 1)) == 255")
	(match_test "INTVAL (XEXP (XEXP (XEXP (XEXP (XEXP (op, 0), 0), 0), 0), 1)) % 8 == 0")
      )
    )
    (match_test "GET_MODE (XEXP (op, 0)) == HImode")
    (match_test "GET_MODE (XEXP (XEXP (op, 0), 0)) == HImode")
    (match_test "GET_MODE (XEXP (XEXP (op, 0), 1)) == HImode")
  )
)

; Match an operator which is true if the carry flag is set.
(define_predicate "carry_flag_operator"
   (and (ior
	   (and (match_code "ltu")
		(match_test "ia16_cc_modes_compatible (CCCmode, GET_MODE (XEXP (op, 0))) == GET_MODE (XEXP (op, 0))"))
	   (and (match_code "leu")
		(match_test "GET_MODE (XEXP (op, 0)) == CCCZ_Cmode"))
	   (and (match_code "gtu")
		(match_test "GET_MODE (XEXP (op, 0)) == CCCZ_NCmode"))
	   (and (match_code "eq")
		(match_test "GET_MODE (XEXP (op, 0)) == CCZ_Cmode"))
	   (and (match_code "ne")
		(match_test "GET_MODE (XEXP (op, 0)) == CCZ_NCmode"))
	   (and (match_code "ge")
		(match_test "GET_MODE (XEXP (op, 0)) == CCS_NCmode"))
	)
	(match_code "reg" "0")
	(match_code "const_int" "1")
	(match_test "REGNO (XEXP (op, 0)) == CC_REG")
	(match_test "XEXP (op, 1) == const0_rtx")
   )
)

; Match an operator which is true if the carry flag is not set.
(define_predicate "carry_not_flag_operator"
   (and (ior
	   (and (match_code "geu")
		(match_test "ia16_cc_modes_compatible (CCCmode, GET_MODE (XEXP (op, 0))) == GET_MODE (XEXP (op, 0))"))
	   (and (match_code "gtu")
		(match_test "GET_MODE (XEXP (op, 0)) == CCCZ_Cmode"))
	   (and (match_code "leu")
		(match_test "GET_MODE (XEXP (op, 0)) == CCCZ_NCmode"))
	   (and (match_code "ne")
		(match_test "GET_MODE (XEXP (op, 0)) == CCZ_Cmode"))
	   (and (match_code "eq")
		(match_test "GET_MODE (XEXP (op, 0)) == CCZ_NCmode"))
	   (and (match_code "lt")
		(match_test "GET_MODE (XEXP (op, 0)) == CCS_NCmode"))
	)
	(match_code "reg" "0")
	(match_code "const_int" "1")
	(match_test "REGNO (XEXP (op, 0)) == CC_REG")
	(match_test "XEXP (op, 1) == const0_rtx")
   )
)
