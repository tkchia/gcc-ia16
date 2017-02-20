/* PR rtl-optimization/64536 */
/* { dg-do link { target { stdint_types } } } */
/* { dg-options "-O2" } */
/* { dg-additional-options "-fPIC" { target fpic } } */
/* { dg-require-effective-target ptr32plus } */

#include <stdint.h>

struct S { intptr_t q; } *h;
intptr_t a, b, g, j, k, *c, *d, *e, *f, *i;
intptr_t *baz (void)
{
  asm volatile ("" : : : "memory");
  return e;
}

void
bar (int x)
{
  int y;
  for (y = 0; y < x; y++)
    {
      switch (b)
	{
	case 0:
	case 2:
	  a++;
	  break;
	case 3:
	  a++;
	  break;
	case 1:
	  a++;
	}
      if (d)
	{
	  f = baz ();
	  g = k++;
	  if (&h->q)
	    {
	      j = *f;
	      h->q = *f;
	    }
	  else
	    i = (intptr_t *) (h->q = *f);
	  *c++ = (intptr_t) f;
	  e += 6;
	}
      else
	{
	  f = baz ();
	  g = k++;
	  if (&h->q)
	    {
	      j = *f;
	      h->q = *f;
	    }
	  else
	    i = (intptr_t *) (h->q = *f);
	  *c++ = (intptr_t) f;
	  e += 6;
	}
    }
}

int
main ()
{
  return 0;
}
