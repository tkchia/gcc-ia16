/* { dg-do compile { target { stdint_types } } } */
/* { dg-options "-Wstrict-aliasing=2 -O2" } */

#include <stdint.h>

float foo ()
{
  uint32_t MASK = 0x80000000;
  return (float &) MASK; /* { dg-warning "strict-aliasing" } */
}

