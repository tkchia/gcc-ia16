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

int maybe_init_again (int *a, int N)
{
  for (int i = 0; i < N; i++)
    a[i] = i;
  return 1;
}

void vec_mult (int *p, int *v1, int *v2, int N)
{
  init (v1, v2, N);

  for (int i = 0; i < N; i++)
    p[i] = v1[i] * v2[i];

  maybe_init_again (v1, N);
  maybe_init_again (v2, N);

  for (int i = 0; i < N; i++)
    p[i] = p[i] + (v1[i] * v2[i]);
}

void vec_mult_1 (int *p, int *v1, int *v2, int N)
{
  init (v1, v2, N);

  #pragma omp target data map(to: v1[:N], v2[:N]) map(from: p[0:N])
    {
      int changed;

      #pragma omp target
        #pragma omp parallel for
	  for (int i = 0; i < N; i++)
	    p[i] = v1[i] * v2[i];

      changed = maybe_init_again (v1, N);
      #pragma omp target update if (changed) to(v1[:N])

      changed = maybe_init_again (v2, N);
      #pragma omp target update if (changed) to(v2[:N])

      #pragma omp target
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
