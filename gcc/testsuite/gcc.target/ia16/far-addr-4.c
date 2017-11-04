/* { dg-do run } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* Arithmetic on a far pointer that causes the offset component to wrap
   around.  This should really just leave the segment component alone, like
   classical IA-16 compilers do.  */

int puts (const char *);
void abort (void);

static inline unsigned long
read_displaced (unsigned long addr, unsigned disp)
{
  unsigned long addr2;

  __asm ("" : "=k" (addr2) : "0" (addr));
  return ((volatile unsigned long __far *) addr2) [disp];
}

int
main (void)
{
  unsigned long w0, w1, w2, w3, w4, w5, w6, w7;

  /* Read longword at 0x0008:0x0000. */
  w0 = *(volatile unsigned long __far *) 0x80000;

  /* Read longword at ((__far *) 0x88000 + 0x8000), which should be the same
     as 0x80000. */
  w1 = read_displaced (0x88000ul, 0x8000u / sizeof (unsigned long));
  if (w1 != w0)
    abort ();
  puts ("w1 == w0");

  /* And so on... */
  w2 = read_displaced (0x88004ul, 0x7ffcu / sizeof (unsigned long));
  if (w2 != w0)
    abort ();
  puts ("w2 == w0");

  w3 = read_displaced (0x80004ul, 0xfffcu / sizeof (unsigned long));
  if (w3 != w0)
    abort ();
  puts ("w3 == w0");

  w4 = read_displaced (0x8ff80ul, 0x80u / sizeof (unsigned long));
  if (w4 != w0)
    abort ();
  puts ("w4 == w0");

  w5 = read_displaced (0x8ff84ul, 0x7cu / sizeof (unsigned long));
  if (w5 != w0)
    abort ();
  puts ("w5 == w0");

  w6 = read_displaced (0x8ff04ul, 0xfcu / sizeof (unsigned long));
  if (w6 != w0)
    abort ();
  puts ("w6 == w0");

  w7 = read_displaced (0x8fffcul, 4u / sizeof (unsigned long));
  if (w7 != w0)
    abort ();
  puts ("w7 == w0");

  return 0;
}
