/* { dg-do compile } */
/* { dg-options "-O2 -fdump-tree-cddce1" } */

#ifdef __INT_LEAST32_TYPE__
#define int_least32_t __INT_LEAST32_TYPE__
#else
#define int_least32_t int
#endif

int_least32_t bar (short *p)
{
  int_least32_t res = *p;
  struct { int_least32_t *q1; int_least32_t *q2; } q;
  q.q1 = __builtin_aligned_alloc (128, 128 * sizeof (int));
  q.q2 = __builtin_aligned_alloc (128, 128 * sizeof (int));
  *q.q1 = 1;
  *q.q2 = 2;
  return res + *p + *q.q1 + *q.q2;
}

/* There should be only one load from *p left.  All stores and all
   other loads should be removed.  Likewise the calls to aligned_alloc.  */

/* { dg-final { scan-tree-dump-times "\\\*\[^ \]" 1 "cddce1" } } */
/* { dg-final { scan-tree-dump-not "aligned_alloc" "cddce1" } } */
