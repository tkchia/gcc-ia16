/* { dg-options "-std=gnu11 --save-temps" } */
/* { dg-additional-sources "support/no-ss-data-main.S" } */
/* { dg-do run } */

/* Test that functions with %ss != .data can set and restore %ds properly.  */

extern void abort (void);

volatile unsigned cu_psp = 0;

__attribute__ ((no_assume_ss_data)) int
do_stuff (void)
{
  unsigned bx;
  __asm volatile ("movw %%ds, %0" : "=b" (bx)
				  : "Rds" (__builtin_ia16_selector (0x17c3)));
  cu_psp = bx;
  __asm volatile ("movw %%es, %0" : "=b" (bx)
				  : "e" (__builtin_ia16_selector (0xe36d)));
  cu_psp += bx;
  return 0;
}

int
check_stuff (void)
{
  if (cu_psp != 0xfb30)
    abort ();

  return 0;
}
