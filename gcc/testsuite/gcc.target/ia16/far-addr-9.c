/* { dg-do assemble } */
/* { dg-options "-O0 --save-temps" } */

/* Test for a pointer arithmetic bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/4 .  */

char __far *inc(char __far *p)
{
  return &p[1];
}
