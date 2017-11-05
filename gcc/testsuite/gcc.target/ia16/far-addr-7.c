/* { dg-do run } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* Using a far pointer to `volatile' loop through an area of (emulated) BIOS
   ROM.  The optimized routine should give the exact same results as the
   unoptimized routine.  Furthermore, reading from 0xfff0:0x0000 should give
   the same results as reading from 0xf000:0xff00.  */

int printf (const char *, ...);

__attribute__ ((optimize ("O3")))
unsigned long
hash1 (void)
{
  volatile unsigned __far *p = (volatile unsigned __far *) 0xfff00000ul;
  unsigned i;
  unsigned long h = 0;
  for (i = 0; i < 0x80u; ++i)
    h = 5 * h ^ *p++;
  return h;
}

__attribute__ ((optimize ("O0")))
unsigned long
hash2 (void)
{
  volatile unsigned __far *p = (volatile unsigned __far *) 0xfff00000ul;
  unsigned i;
  unsigned long h = 0;
  for (i = 0; i < 0x80u; ++i)
    h = 5 * h ^ *p++;
  return h;
}

unsigned long
hash3 (void)
{
  volatile unsigned __far *p = (volatile unsigned __far *) 0xf000ff00ul;
  unsigned i;
  unsigned long h = 0;
  for (i = 0; i < 0x80u; ++i)
    h = 5 * h ^ *p++;
  return h;
}

int
main (void)
{
  unsigned long h1 = hash1 (), h2 = hash2 (), h3 = hash3 ();
  printf ("%#lx %#lx %#lx\n", h1, h2, h3);
  return h1 == h2 && h1 == h3 ? 0 : 1;
}
