/* { dg-do run { target { stdint_types } } } */

#include <stdint.h>

extern "C" void abort() __attribute__ ((noreturn));

struct s
{
  unsigned long long f1 : 40;
  uint_least32_t f2 : 24;
} sv;

int main()
{
  int_least32_t f2;
  sv.f2 = ((uint_least32_t)1 << 24) - 1;
  __asm__ volatile ("" : : : "memory");
  ++sv.f2;
  f2 = sv.f2;
  if (f2 != 0)
    abort();
  return 0;
}
