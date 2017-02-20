#include <stdint.h>

struct S
{
  static int_least32_t i;
  static void set (int_least32_t ii) { i = -ii; }
};
