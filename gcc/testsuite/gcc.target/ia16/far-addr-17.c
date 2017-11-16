/* { dg-do assemble } */
/* { dg-options "-Os --save-temps" } */

/* Test for a far pointer processing bug reported by Bart Oldeman --- see
   https://github.com/tkchia/gcc-ia16/issues/8 .

   GCC's induction variable optimization pass (`-fivopts') sometimes creates
   nonsense "far" addresses --- with no segment override --- and passes them
   to TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS, in order to estimate the costs
   of using various types of addresses.  So we need to make sure that
   TARGET_ADDR_SPACE_LEGITIMIZE_ADDRESS can gracefully handle such things.  */

unsigned my_strlen(char __far * fname)
{
  unsigned length = 0;
  while (fname[length]) length++;
  return length;
}
