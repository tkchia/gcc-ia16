/* { dg-do run { target { stdint_types } } } */

#include <stdint.h>

extern "C" void abort (void);

struct s
{
  unsigned long long f1 : 40;
  uint_least32_t f2 : 24;
};

s sv;

void __attribute__((noinline)) foo(uint_least32_t i)
{
  uint_least32_t tmp;
  sv.f2 = i;
  tmp = sv.f2;
  if (tmp != 0)
    abort ();
}

int main()
{
  foo (0xff000000u);
  return 0;
}
