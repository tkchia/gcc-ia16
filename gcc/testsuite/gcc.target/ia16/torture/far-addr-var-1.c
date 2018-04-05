/* { dg-options "-mcmodel=small -std=gnu11 --save-temps" } */
/* { dg-do run } */

/* Test that we can create far variables, and also fabricate pointers to
   them, when using the small memory model.  */

int printf (const char *, ...);
void abort (void);

__attribute__ ((noinline)) int __far *
p_var (void)
{
  static int __far var = 1;
  return &var;
}

int main (void)
{
  printf ("%lx\n", (unsigned long) p_var ());

  if (*p_var () != 1)
    abort ();

  return 0;
}
