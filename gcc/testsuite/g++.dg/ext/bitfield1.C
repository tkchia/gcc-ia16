// PR c++/30328
// { dg-do link { target { stdint_types } } }
// { dg-options "" }

#include <stdint.h>

struct S
{
  int_least32_t a:17;
} x;

typedef typeof (x.a) foo;

template <class T>
T* inc(T* p) { return p+1; }

int main ()
{
  foo x[2] = { 1,2 };
  int_least32_t y[2] = { 1,2 };
  *inc(x);
  *inc(y);
  return 0;
}
