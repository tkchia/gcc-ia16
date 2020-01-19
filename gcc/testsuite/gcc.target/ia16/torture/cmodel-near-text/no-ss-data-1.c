/* { dg-options "-std=gnu11 --save-temps" } */
/* { dg-additional-sources "support/no-ss-data-main.S" } */
/* { dg-do run } */

/* Test that functions with %ss != .data can properly handle initializers of
   local variables, and `switch' statements with jump tables in %ds.  */

extern void abort (void);

__attribute__ ((no_assume_ss_data, noinline)) static unsigned
fib (unsigned i)
{
  switch (i)
    {
    case 0:
      return 0;
    case 1:
    case 2:
      return 1;
    case 3:
      return 2;
    case 4:
      return 3;
    case 5:
      return 5;
    case 6:
      return 8;
    case 7:
      return 13;
    case 8:
      return 21;
    case 9:
      return 34;
    case 10:
      return 55;
    case 11:
      return 89;
    case 12:
      return 144;
    case 13:
      return 233;
    case 14:
      return 377;
    case 15:
      return 610;
    default:
      abort ();
    }
}

__attribute__ ((no_assume_ss_data)) int
do_stuff (void)
{
  volatile char blah[] = "gaza frequens libycos duxit karthago triumphos";
  char blurfl[] = "the five boxing wizards jump quickly";

  if (fib (11) != 89)
    abort ();

  if (blah[fib (8)] != blah[fib (7)])
    abort ();

  if (blah[fib (9)] != 'g')
    abort ();

  if (blurfl[fib (9)] != blah[14])
    abort ();

  return 0;
}

int
check_stuff (void)
{
  return 0;
}
