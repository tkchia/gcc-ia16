/* { dg-do compile } */
/* { dg-options "-O1 --save-temps" } */

/* Test for a far pointer processing bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/2 .  */

struct string_int {
  char str[1];
  int number;
};

int p_number_not_zero(struct string_int __far *p)
{
  return p->number != 0;
}
