/* { dg-do compile { target { stdint_types } } } */
/* { dg-options "-O2 -fdump-tree-vrp2" } */
/* { dg-require-effective-target int32plus } */

#include <stdint.h>

int_least32_t Foo (int_least32_t X)
{
  if (X < 0)
    X = 0;
  if (X > 191)
    X = 191;

  return X << 23;
}

/* We expect this min/max pair to survive.  */

/* { dg-final { scan-tree-dump-times "MIN_EXPR" 1 "vrp2" } } */
/* { dg-final { scan-tree-dump-times "MAX_EXPR" 1 "vrp2" } } */
