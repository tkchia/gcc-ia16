/* { dg-do run } */
/* { dg-require-effective-target size32plus } */
/* { dg-require-effective-target int32plus } */
/* { dg-options "-O2 -floop-interchange -ffast-math -fno-ipa-cp" } */

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#endif

#ifdef __ia16__
#define SIZE 50
#else
#define SIZE 1335
#endif

double u[SIZE*SIZE];

static int __attribute__((noinline))
foo (int N, int *res)
{
  int i, j;
  double sum = 0;
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
      sum = sum + u[i + SIZE * j];

  for (i = 0; i < N; i++)
    u[(SIZE + 1) * i] *= 2;

  *res = sum + N + u[(SIZE + 1) * 2] + u[SIZE + 1];
}

extern void abort ();

int
main (void)
{
  int i, j, res;

  for (i = 0; i < SIZE*SIZE; i++)
    u[i] = 2;

  foo (SIZE, &res);

#if DEBUG
  fprintf (stderr, "res = %d \n", res);
#endif

  if (res != 2*SIZE*SIZE + SIZE + 4 + 4)
    abort ();

  return 0;
}
