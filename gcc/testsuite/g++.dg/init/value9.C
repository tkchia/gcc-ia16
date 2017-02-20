// PR c++/50793
// { dg-do run { target { stdint_types } } }

#include <stdint.h>

struct NonTrivial
{
  NonTrivial() { }
};

struct S
{
  NonTrivial nt;
  int_least32_t i;
};

int_least32_t f(S s)
{
  s.i = 0xdeadbeef;
  return s.i;
}

int_least32_t g(S s = S())
{
  return s.i;
}

int main()
{
  f(S());  // make stack dirty

  if ( g() )
    __builtin_abort();
}
