/* { dg-do assemble } */
/* { dg-options "-Og --save-temps" } */

/* Second test case for a pointer processing bug reported by Bart Oldeman
   --- see https://github.com/tkchia/gcc-ia16/issues/7 .  */

char p_i(char __far *p, int i)
{
  return p[i];
}
