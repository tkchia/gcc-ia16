// { dg-do run }
// { dg-additional-options "-flto" }
// { dg-require-effective-target target_device }

#include <omp.h>

#define N 100000

extern "C" void abort (void);

void init (int *a1, int *a2)
{
  int s = -1;
  for (int i = 0; i < N; i++)
    {
      a1[i] = s;
      a2[i] = i;
      s = -s;
    }
}

void check (int *a, int *b)
{
  for (int i = 0; i < N; i++)
    if (a[i] != b[i])
      abort ();
}

void vec_mult (int *p)
{
  int v1[N], v2[N];

  init (v1, v2);

  for (int i = 0; i < N; i++)
    p[i] = v1[i] * v2[i];
}

void vec_mult_1 (int *p)
{
  int v1[N], v2[N];

  init (v1, v2);

  #pragma omp target map(p[0:N])
    #pragma omp parallel for
      for (int i = 0; i < N; i++)
	p[i] = v1[i] * v2[i];
}

void vec_mult_2 (int *p)
{
  int v1[N], v2[N];

  init (v1, v2);

  #pragma omp target map(from: p[0:N])
    #pragma omp parallel for
      for (int i = 0; i < N; i++)
	p[i] = v1[i] * v2[i];
}

void vec_mult_3 (int *p)
{
  int v1[N], v2[N];

  init (v1, v2);

  #pragma omp target map(v1, v2, p[0:N])
    #pragma omp parallel for
      for (int i = 0; i < N; i++)
	p[i] = v1[i] * v2[i];
}

void vec_mult_4 (int *p, int *v1, int *v2)
{
  #pragma omp target map(to: v1[0:N], v2[:N]) map(from: p[0:N])
    #pragma omp parallel for
      for (int i = 0; i < N; i++)
	p[i] = v1[i] * v2[i];
}

int main ()
{
  int p[N], p1[N], p2[N], p3[N], p4[N];
  int v1[N], v2[N];

  init (v1, v2);

  vec_mult (p);
  vec_mult_1 (p1);
  vec_mult_2 (p2);
  vec_mult_3 (p3);
  vec_mult_4 (p4, v1, v2);

  check (p, p1);
  check (p, p2);
  check (p, p3);
  check (p, p4);

  return 0;
}
