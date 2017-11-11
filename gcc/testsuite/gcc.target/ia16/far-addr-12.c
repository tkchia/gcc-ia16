/* { dg-do compile } */
/* { dg-options "-O1 --save-temps" } */

/* Test for a pointer processing bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/6 .  */

struct x2 {
  char x[2];
} psp;

int get_p_x_i(struct x2 __far *p, int i)
{
  return p->x[i];
}
