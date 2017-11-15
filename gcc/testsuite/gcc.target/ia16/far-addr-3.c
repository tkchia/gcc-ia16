/* { dg-do assemble } */
/* { dg-xfail-if "" *-*-* } */
/* { dg-excess-errors "" } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* FAIL: Can we create far address space variables and take their addresses? 
   At the moment, this is not supported.  */

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
