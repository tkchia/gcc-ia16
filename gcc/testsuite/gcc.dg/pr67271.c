/* { dg-do compile { target { stdint_types } } } */
/* { dg-options "-O" } */
/* { dg-require-effective-target int32plus } */

#include <stdint.h>

extern long int labs (long int j);
int
main ()
{
  long *a = (long *)"empty";
  int_least32_t i = 1441516387;
  a[i] = labs (a[i]);
  return 0;
}
