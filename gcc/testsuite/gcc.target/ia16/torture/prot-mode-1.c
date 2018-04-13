/* { dg-options "-mcmodel=small -std=gnu11 --save-temps" } */
/* { dg-do assemble } */

/* Test that the __builtin_ia16_selector (.) function allows a value to be
   loaded into a segment register for an __asm (...), whether or not
   -mprotected-mode is enabled.  */

void
foo (unsigned sel1, unsigned sel2)
{
  __asm volatile ("" : : "Q" (__builtin_ia16_selector (sel1)),
			 "Q" (__builtin_ia16_selector (sel2)));
}
