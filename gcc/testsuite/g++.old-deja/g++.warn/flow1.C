// { dg-do assemble { target { stdint_types } } }
// { dg-options "-Wreturn-type" }
// Test that we don't get a warning about flowing off the end.

#include <stdint.h>

struct A {
  ~A ();
};

int_least32_t f()
{
  A a1[2];
  A a2[2];
  return 1234567;
}
