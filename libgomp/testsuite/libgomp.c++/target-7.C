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

void init_again (int *a1, int *a2, int N)
{
  int s = -1;
  for (int i = 0; i < N; i++)
    {
      a1[i] = s * 10;
      a2[i] = i;
      s = -s;
    }
}

void vec_mult (int *p, int *v1, int *v2, int N)
{
  init (v1, v2, N);

  for (int i = 0; i < N; i++)
    p[i] = v1[i] * v2[i];

  init_again (v1, v2, N);

  for (int i = 0; i < N; i++)
    p[i] = p[i] + (v1[i] * v2[i]);
}

void vec_mult_1 (int *p, int *v1, int *v2, int N)
{
  init (v1, v2, N);

  #pragma omp target data map(from: p[0:N])
    {
      #pragma omp target map(to: v1[:N], v2[:N])
	#pragma omp parallel for
	  for (int i = 0; i < N; i++)
	    p[i] = v1[i] * v2[i];

      init_again (v1, v2, N);

      #pragma omp target map(to: v1[:N], v2[:N])
	#pragma omp parallel for
	  for (int i = 0; i < N; i++)
	    p[i] = p[i] + (v1[i] * v2[i]);
    }
}

int main ()
{
  int *p = new int[MAX];
  int *p1 = new int[MAX];
  int *v1 = new int[MAX];
  int *v2 = new int[MAX];

  vec_mult (p, v1, v2, MAX);
  vec_mult_1 (p1, v1, v2, MAX);

  check (p, p1, MAX);

  delete [] p;
  delete [] p1;
  delete [] v1;
  delete [] v2;

  return 0;
}
