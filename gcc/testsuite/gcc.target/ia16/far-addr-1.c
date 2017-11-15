/* { dg-do assemble } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* Test for basic far pointer functionality.  Also test that the preprocessor
   macro __FAR is defined.  */

unsigned long
irq0_clock_ticks (void)
{
#ifdef __FAR
  volatile unsigned long __far *p =
    (volatile unsigned long __far *) 0x0040006C;
  return *p;
#else
# error
#endif
}
/* { dg-final { scan-assembler "movw\[ \\t\]%\[de\]s:" } } */
