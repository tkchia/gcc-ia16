/* { dg-do compile } */
/* { dg-options "-O1 --save-temps" } */

/* Test for a pointer processing bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/5 .  */

void scan(int __far *p)
{
  for(; *p; p++);
}
