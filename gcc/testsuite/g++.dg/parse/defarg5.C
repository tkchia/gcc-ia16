// { dg-do compile { target { stdint_types } } }

// Copyright (C) 2003 Free Software Foundation, Inc.
// Contributed by Nathan Sidwell 5 Sep 2003 <nathan@codesourcery.com>
// Origin:Wolfgang Bangerth bangerth@dealii.org

// PR c++/12167 - infinite recursion

#include <stdint.h>

class A {   
  void report(int_least32_t d
	      // the default arg is what NAN etc can expand to, but
	      // with the floatiness removed.
	      = (__extension__ ((union { uint_least32_t l; int_least32_t d; })
				{ l: 0x7fc00000U }).d));
};
