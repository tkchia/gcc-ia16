// { dg-do compile { target c++14 } }

#include <stdint.h>

struct Foo
{
  uint_least32_t i: 32;
};

int
main()
{
  Foo f{};
  return f.i;
}
