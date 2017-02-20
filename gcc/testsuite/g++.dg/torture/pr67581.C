/* { dg-do compile { target { stdint_types } } } */

#include <stdint.h>

union U 
{
  int32_t x; 
  float y;
} __attribute__ ((__transparent_union__));
