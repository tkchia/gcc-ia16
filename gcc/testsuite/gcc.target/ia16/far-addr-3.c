/* { dg-do assemble } */
/* { dg-xfail-if "" *-*-* } */
/* { dg-excess-errors "" } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* XFAIL: Do not allow far address space variables to be created, if we are
   using the default tiny memory model.  */

int printf (const char *, ...);

static int __far var = 1;

int __far *
p_var (void)
{
  return &var;
}

int main (void)
{
  printf ("%lx\n", (unsigned long) p_var ());
  return 0;
}
