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

static inline unsigned long
read_neg_displaced (unsigned long addr, unsigned disp)
{
  unsigned long addr2;

  __asm ("" : "=k" (addr2) : "0" (addr));
  return *((volatile unsigned long __far *) addr2 - disp);
}

int
main (void)
{
  unsigned long w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13;

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

  /* Read longword at ((__far *) 0 - 0xff80), which should be the same
     as 0x80, and 0:0x80 == 0x8:0.  */
  w8 = read_neg_displaced (0ul, 0xff80u / sizeof (unsigned long));
  if (w8 != w0)
    abort ();
  puts ("w8 == w0");

  /* And so on... */
  w9 = read_neg_displaced (4ul, 0xff84u / sizeof (unsigned long));
  if (w9 != w0)
    abort ();
  puts ("w9 == w0");

  w10 = read_neg_displaced (8ul, 0xff88u / sizeof (unsigned long));
  if (w10 != w0)
    abort ();
  puts ("w10 == w0");

  w11 = read_neg_displaced (0x10ul, 0xff90u / sizeof (unsigned long));
  if (w11 != w0)
    abort ();
  puts ("w11 == w0");

  w12 = read_neg_displaced (0x20ul, 0xffa0u / sizeof (unsigned long));
  if (w12 != w0)
    abort ();
  puts ("w12 == w0");

  w13 = read_neg_displaced (0x40ul, 0xffc0u / sizeof (unsigned long));
  if (w13 != w0)
    abort ();
  puts ("w13 == w0");

  return 0;
}
