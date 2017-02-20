// PR c++/70001

// This is still slow to compile, only run it once.
// { dg-do compile { target c++14_only } }
// { dg-skip-if "Does not fit in 64kB" { "ia16-*-*" } }

struct B
{
  int a;
  constexpr B () : a (0) { }
};

struct A
{
  B b[1 << 19][1][1][1];
} c;
