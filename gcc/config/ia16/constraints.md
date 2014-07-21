;  Constraint definitions for Intel 16-bit x86.
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


; Predefined constraints:
;          E Fg  i       m n o p   r s      V   X    <>0123456789

; Machine specific integer constant constraints:
; Available:
;                   J K     N O

; Machine specific floating point constant constraints:
; Available:
;             G H

; Machine specific constraints:
; Used register constraints:
; aAbBc dD     h   j k l         qQ R S Tu   w x
; Used constant constraints (IJKLMNOP integers only):
;                 I     L M     P         U         Z
; Available:
;      C  e f                          t   v  W  yYz

; Individual 16-bit registers.
(define_register_constraint "a"	  "AX_REGS"	"The @code{ax} register.")
(define_register_constraint "b"	  "BX_REGS"	"The @code{bx} register.")
(define_register_constraint "c"	  "CX_REGS"	"The @code{cx} register.")
(define_register_constraint "d"   "DX_REGS"	"The @code{dx} register.")
(define_register_constraint "S"	  "SI_REGS"	"The @code{si} register.")
(define_register_constraint "D"	  "DI_REGS"	"The @code{di} register.")
(define_register_constraint "Rbp" "BP_REGS"	"The @code{bp} register.")

; Individual 8-bit registers.
(define_register_constraint "Ral" "AL_REGS"	"The @code{al} register.")
(define_register_constraint "Rah" "AH_REGS"	"The @code{ah} register.")
(define_register_constraint "Rcl" "CL_REGS"	"The @code{cl} register.")
(define_register_constraint "Rdl" "DL_REGS"	"The @code{dl} register.")
(define_register_constraint "Rdh" "DH_REGS"	"The @code{dh} register.")

; General purpose register classes.
(define_register_constraint "T"	"SEG_GENERAL_REGS"
	"All general and segment registers.")
(define_register_constraint "q" "QI_REGS"
	"The 8-bit registers.")
(define_register_constraint "l"	"LO_QI_REGS"
	"The lower half of pairs of 8-bit registers.")
(define_register_constraint "u"	"UP_QI_REGS"
	"The upper half of pairs of 8-bit registers.")
(define_register_constraint "B"	"BASE_REGS"
	"The @code{bx}, @code{si}, @code{di} and @code{bp} registers.")
(define_register_constraint "w"	"BASE_W_INDEX_REGS"
	"The @code{bx} and @code{bp} registers.")
(define_register_constraint "x"	"INDEX_REGS"
	"The @code{si} and @code{di} registers.")

; Special groups of registers.
(define_register_constraint "A"	"DXAX_REGS"  "The @code{dx:ax} register pair.")
(define_register_constraint "j"	"BXDX_REGS"  "The @code{bx:dx} register pair.")
(define_register_constraint "Q" "SEGMENT_REGS"	"The @code{es} register.")
(define_register_constraint "k"	"QISI_REGS"
	"SImode registers with two 8-bit low parts.")

; Constraints for integer constants.
(define_constraint	"Z"	"The constant 0."
   (and (match_code "const_int,const_vector")
	(match_test "op == CONST0_RTX (mode)")))

(define_constraint	"P1"	"The constant 1."
   (and (match_code "const_int")
	(match_test "ival == 1")))

(define_constraint	"M1"	"The constant @minus{}1."
   (and (match_code "const_int")
	(match_test "ival == -1")))

; An upper byte mask constant.
(define_constraint	"Um"	"The constant @minus{}256."
   (and (match_code "const_int")
	(match_test "ival == -256")))

; A lower byte mask constant.
(define_constraint	"Lbm"	"The constant 255."
   (and (match_code "const_int")
	(match_test "ival == 255")))

; Low part constants for IOR and XOR with a register operand.
; 0x0080 (128) to 0x00FE (254).
(define_constraint	"Lor"	"Constants 128 @dots{} 254."
   (and (match_code "const_int")
	(match_test "ival >= 128 && ival <= 254")))

; Low part constants for IOR and XOR with a memory operand.
; 0x0001 (1) to 0x00FE (254).
(define_constraint	"Lom"	"Constants 1 @dots{} 254."
   (and (match_code "const_int")
	(match_test "ival >= 1 && ival <= 254")))

; Low part constants for AND with a register operand.
; 0xFF01 (-255) to 0xFF7F (-129).
(define_constraint	"Lar"	"Constants @minus{}255 @dots{} @minus{}129."
   (and (match_code "const_int")
	(match_test "ival >= -255 && ival <= -129")))

; Low part constants for AND with a memory operand.
; 0xFF01 (-255) to 0xFFFE (-2).
(define_constraint	"Lam"	"Constants @minus{}255 @dots{} @minus{}2."
   (and (match_code "const_int")
	(match_test "ival >= -255 && ival <= -2")))

; High part constants for IOR, XOR, PLUS and MINUS.
(define_constraint	"Uo"	"Constants 0xXX00 except @minus{}256."
   (and (match_code "const_int")
	(match_test "ival != -256 && (ival & 0xFF) == 0")))

; High part constants for AND.
; Note that this matches -1 too, but is hopefully optimized away.
(define_constraint	"Ua"	"Constants 0xXXFF."
   (and (match_code "const_int")
	(match_test "(ival & 0xFF) == 0xFF")))

; Supported immediate shift counts.
(define_constraint	"Ish"	"A constant usable as a shift count."
   (and (match_code "const_int")
	(match_test "TARGET_SHIFT_IMM ? true : ival == 1")))

; Supported immediate multipliers for "aad".
(define_constraint	"Iaa"	"A constant multiplier for the @code{aad} instruction."
   (and (match_code "const_int")
	(match_test "TARGET_AAD_IMM ? true : ival == 10")))

; Supported immediate operands for "push".
(define_constraint	"Ipu"	"A constant usable with the @code{push} instruction."
   (and (match_code "const_int")
	(match_test "TARGET_PUSH_IMM ? true : false")))

; Supported immediate operands for "imul" except 257.
(define_constraint	"Imu"	"A constant usable with the @code{imul} instruction except 257."
   (and (match_code "const_int")
	(match_test "TARGET_IMUL_IMM ? ival != 257 : false")))

; Multiplying by 257 is cheap. High and low byte are both 1.
(define_constraint	"I11"	"The constant 257."
   (and (match_code "const_int")
	(match_test "ival == 257")))
