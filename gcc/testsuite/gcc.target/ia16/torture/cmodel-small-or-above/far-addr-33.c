/* { dg-options "-std=gnu11 --save-temps" } */
/* { dg-do run } */

/*
 * Test that __builtin_ia16_static_far_cast (.) works, and that we can
 * dereference a pointer to non-void returned by the function.
 *
 * Issue report and portions of test case originally submitted by Adrian
 * Siekierka --- see https://codeberg.org/tkchia/gcc-ia16/issues/15 .
 */

struct a
{
  void __far *ptr;
};

struct b
{
  int value1, value2;
};

struct b instance_b = { 1234, 5678 };
struct a instance_a1
  = {
      __builtin_ia16_static_far_cast (&instance_b)
    };
struct a instance_a2
  = {
      &__builtin_ia16_static_far_cast (&instance_b)->value2
    };

void abort (void);

int
main (void)
{
  if (instance_a1.ptr != &instance_b)
    abort ();

  if (instance_a2.ptr != &instance_b.value2)
    abort ();

  if (*(int __far *) instance_a2.ptr != 5678)
    abort ();

  return 0;
}
