/* { dg-options "-mcmodel=small -std=gnu11 --save-temps" } */
/* { dg-do run } */

/* Test that we can manipulate far variables (even without creating pointers
   to them), when using the small code model.  */

void abort (void);

int __far var = 1;

__attribute__ ((noinline)) int
foo (void)
{
  ++var;
  return var;
}

int main (void)
{
  if (foo () != 2)
    abort ();

  if (foo () != 3)
    abort ();

  if (foo () != 4)
    abort ();

  if (foo () != 5)
    abort ();

  return 0;
}
