/* { dg-options "-mcmodel=small -std=gnu11 --save-temps" } */
/* { dg-do run } */

/* Can we statically initialize far pointers to point to specific far
   variables, under the small code model?  */

void abort (void);

int __far y = 42;
const long __far z = 1;
double __far w = 3.0;
int __far foofoo = 6;
int __far *py = &y;
int __far *py1 = &y + 1;
int __far *pym1 = &y - 1;

__attribute__ ((noinline)) char __far *
f (void)
{
  static char __far x = 1;
  static char __far *px = &x;
  x -= 2;
  return px + x;
}

int main (void)
{
  if (f () [1] != -1)
    abort ();

  if (f () [3] != -3)
    abort ();

  if (f () [5] != -5)
    abort ();

  if (f () [7] != -7)
    abort ();

  if (py1[-1] != 42)
    abort ();

  if (pym1[1] != 42)
    abort ();

  return 0;
}
