// { dg-do run }
// { dg-additional-options "-flto" }
// { dg-require-effective-target target_device }

#include <omp.h>

const int MAX = 1800;

extern "C" void abort (void);

void check (int *a, int *b, int N)
{
  for (int i = 0; i < N; i++)
    if (a[i] != b[i])
      abort ();
}

void init (int *a1, int *a2, int N)
{
  int s = -1;
  for (int i = 0; i < N; i++)
    {
      a1[i] = s;
      a2[i] = i;
      s = -s;
    }
}

void vec_mult (int *p1, int *v3, int *v4, int N)
{
  for (int i = 0; i < N; i++)
    p1[i] = v3[i] * v4[i];
}

void foo (int *p0, int *v1, int *v2, int N)
{
  init (v1, v2, N);
  vec_mult (p0, v1, v2, N);
}

void vec_mult_1 (int *p1, int *v3, int *v4, int N)
{
  #pragma omp target map(to: v3[0:N], v4[:N]) map(from: p1[0:N])
    #pragma omp parallel for
      for (int i = 0; i < N; i++)
	p1[i] = v3[i] * v4[i];
}

void foo_1 (int *p0, int *v1, int *v2, int N)
{
  init (v1, v2, N);

  #pragma omp target data map(to: v1[0:N], v2[:N]) map(from: p0[0:N])
    {
      vec_mult_1 (p0, v1, v2, N);
    }
}

int main ()
{
  int *p = new int[MAX];
  int *p1 = new int[MAX];
  int *v1 = new int[MAX];
  int *v2 = new int[MAX];

  foo (p, v1, v2, MAX);
  foo_1 (p1, v1, v2, MAX);

  check (p, p1, MAX);

  delete [] p;
  delete [] p1;
  delete [] v1;
  delete [] v2;

  return 0;
}
