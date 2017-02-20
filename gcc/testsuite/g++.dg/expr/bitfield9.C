// PR c++/32346
// { dg-do run { target { stdint_types } } }
// { dg-options "-Wno-overflow" }

#include <stdint.h>

extern "C" void abort();

struct S {
  long long i : 32;
};

void f(int_least32_t i, int j) {
  if (i != 0xabcdef01)
    abort();
  if (j != 0)
    abort();
}

void g(S s) {
  f(s.i, 0);
}

int main() {
  S s;
  s.i = 0xabcdef01;
  g(s);
}
