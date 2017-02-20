// { dg-do compile { target c++14 } }

#include <stdint.h>

template<typename T>
  constexpr int_least32_t var = sizeof (T);

template<>
  constexpr int_least32_t var<int> = 100000;

int main ()
{
  static_assert(var<int> == 100000 && var<char> == sizeof(char), "");
}
