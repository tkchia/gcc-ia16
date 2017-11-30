/* { dg-do assemble } */
/* { dg-options "-Os --save-temps" } */

/* FAIL: Can we copy a moderately big structure from one far address to
   another?  See https://github.com/tkchia/gcc-ia16/issues/12 .  */

typedef struct { char f[64]; } T;

void f (T __far *p, T __far *q)
{
  *p = *q;
}
