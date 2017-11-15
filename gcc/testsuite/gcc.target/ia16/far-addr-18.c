/* { dg-do assemble } */
/* { dg-options "-Os --save-temps" } */

/* Test for a far pointer processing bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/9 .

   (And thus did the local register allocation (LRA) pass say to me, "d00d,
   you're doing it wrong.")  -- tkchia 20171114  */

struct int_struct
{
  int i;
};

void incp(struct int_struct __far *p)
{
  p->i++;
}
