// { dg-do run }
// { dg-additional-options "-flto" }
// { dg-require-effective-target target_device }

#include <omp.h>

extern "C" void abort (void);

int main ()
{
  int a = 100;
  int b, c, d;

  #pragma omp target if(a > 200 && a < 400)
    b = omp_is_initial_device ();

  a += 200;

  #pragma omp target if(a > 200 && a < 400)
    c = omp_is_initial_device ();

  a += 200;

  #pragma omp target if(a > 200 && a < 400)
    d = omp_is_initial_device ();

  if (!b || c || !d)
    abort ();

  return 0;
}
