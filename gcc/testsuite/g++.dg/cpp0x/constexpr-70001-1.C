// PR c++/70001
// { dg-do compile { target c++11 } }
// { dg-skip-if "Does not fit in 64kB" { "ia16-*-*" } }

struct B
{
  int a;
  constexpr B () : a (0) { }
};

struct A
{
  B b[1 << 19];
} c;
