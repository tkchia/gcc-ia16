/* { dg-do assemble } */
/* { dg-options "-Os --save-temps" } */

/* Test for a far pointer + induction variable optimization bug reported by
   Bart Oldeman --- see https://github.com/tkchia/gcc-ia16/issues/10 .  */

const char __far *get_end(const char __far * fname, unsigned length)
{
  while (length && *++fname)
    length--;
  return fname;
}

void pad_spaces(char __far *s)
{
  int i;
  /* pad with spaces */
  for (i = 10; s[i] == '\0'; i--)
    s[i] = ' ';
}
