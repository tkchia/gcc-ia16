/* { dg-do assemble } */
/* { dg-options "-Os -fcall-used-es --save-temps" } */

/* Second test for a far pointer processing bug reported by Bart Oldeman ---
   see https://github.com/tkchia/gcc-ia16/issues/14 .  */

extern int foo(int, int);

struct ab {
  int a, b;
};

int bar(struct ab __far * x)
{
  return 16 % x->b;
}
