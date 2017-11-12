/* { dg-do compile } */
/* { dg-options "-Os --save-temps" } */

/* First test case for a pointer processing bug reported by Bart Oldeman ---
   see https://github.com/tkchia/gcc-ia16/issues/7 .  */

struct x2 {
  char x[2];
} psp;

extern void foo(char __far *p);

void foo_p_x_i(struct x2 __far *p, int i)
{
  foo(&p->x[i]);
}
