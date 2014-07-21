/* { dg-do compile } */
/* { dg-options "-O2 -std=c99" } */
/* { dg-skip-if "Array too large" { ia16-*-* } { "*" } { "" } } */

char heap[50000];

int
main ()
{
  for (unsigned ix = sizeof (heap); ix--;)
    heap[ix] = ix;
  return 0;
}
