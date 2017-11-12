/* { dg-do compile } */
/* { dg-xfail-if "" *-*-* } */
/* { dg-excess-errors "" } */
/* { dg-options "-O0 --save-temps" } */

/* FAIL: Casting an array within a far structure to a pointer.  Bug was
   reported by Bart Oldeman --- https://github.com/tkchia/gcc-ia16/issues/3 .

   A workaround for now is to write the function call as something like
   `farstr(&p->str[0]);' .

   About the bug: apparently somehow, at some point in the compilation, the
   fact that the array `p->str' is in the far address space gets lost.  GCC
   thus thinks that the array -> pointer conversion also needs to cast a
   generic (near) pointer to a far pointer, leading to weirdness.

   This seems to be a bug in the type logic of GCC's machine-independent
   GENERIC tree code.  If so, it may well apply to other GCC ports that
   support named address spaces.  */

struct string1 {
  char str[1];
};

void farstr(char __far *);

void call_farstr(struct string1 __far *p)
{
  farstr(p->str);
}
