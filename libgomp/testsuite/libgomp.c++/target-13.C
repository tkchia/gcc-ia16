// { dg-do run }
// { dg-additional-options "-flto" }
// { dg-require-effective-target target_device }

#include <omp.h>

#define THRESHOLD 20

extern "C" void abort (void);

#pragma omp declare target
int fib (int n)
{
  if (n <= 0)
    return 0;
  else if (n == 1)
    return 1;
  else
    return fib (n-1) + fib (n-2);
}
#pragma omp end declare target

int fib_wrapper (int n)
{
  int x = 0;

  #pragma omp target if(n > THRESHOLD)
    x = fib (n);

  return x;
}

int main ()
{
  if (fib (15) != fib_wrapper (15))
    abort ();
  if (fib (25) != fib_wrapper (25))
    abort ();
  return 0;
}
