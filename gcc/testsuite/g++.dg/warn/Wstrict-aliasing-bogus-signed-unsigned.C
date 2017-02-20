/* { dg-do compile { target { stdint_types } } } */
/* { dg-options "-O2 -Wstrict-aliasing -fstrict-aliasing" } */

#include <stdint.h>

int32_t foo () {
  int32_t i;
  uint32_t* pu = reinterpret_cast<uint32_t*> (&i);  /* { dg-bogus "signed vs. unsigned" } */
  *pu = 1000000;
  return i;
}
