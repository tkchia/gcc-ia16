// PR middle-end/60597
// { dg-do compile { target { stdint_types } } }
// { dg-options "-O2 -g" }

#include <stdint.h>

struct A
{
  int_least32_t foo () const;
  int_least32_t bar () const;
  int_least32_t a;
};

struct B
{
  int_least32_t foo ();
  int_least32_t bar ();
};

int_least32_t *c, d;

int_least32_t
A::foo () const
{
  int_least32_t b = a >> 16;
  return b;
}

int_least32_t
A::bar () const
{
  int_least32_t b = a;
  return b;
}

void
baz (A &x, B h, int_least32_t i, int_least32_t j)
{
  for (; i < h.bar (); ++i)
    for (; h.foo (); ++j)
      {
	int_least32_t g = x.foo ();
	int_least32_t f = x.bar ();
	int_least32_t e = c[0] & 1;
	d = (e << 1) | (g << 16) | (f & 1);
	c[j] = 0;
      }
}
