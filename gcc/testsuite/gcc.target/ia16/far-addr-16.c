/* { dg-do run } */
/* { dg-options "-mcmodel=tiny -O3 -fno-inline --save-temps" } */

/* Check if far pointers wrap around right (i.e. only the offsets wrap) when
   we use pointer arithmetic to access fields in far structures.  */

#define MK_FP(seg, off) \
	((void __far *) ((unsigned long) (unsigned) (seg) << 16 | \
					 (unsigned) (off)))
#define FLUFF_AMOUNT	0x0badu

int printf (const char *, ...);
void abort (void);

struct s1
{
  unsigned p, q, r, s;
};

struct s2
{
  unsigned fluff[FLUFF_AMOUNT], p, q, r, s;
};

static unsigned
ss (void)
{
  unsigned x;
  __asm ("movw %%ss, %0" : "=r" (x));
  return x;
}

void __far *
addr_1 (void)
{
  return MK_FP (0xf000, 0xff00);
}

void __far *
addr_2 (void)
{
  return MK_FP (0xfff0, -FLUFF_AMOUNT * sizeof (unsigned));
}

void __far *
addr_3 (void)
{
  return MK_FP (ss (), 0x100);
}

void __far *
addr_4 (void)
{
  return MK_FP (ss (), 0x100 - FLUFF_AMOUNT * sizeof (unsigned));
}

int
main (void)
{
  struct s1 __far *as1 = addr_1 ();
  struct s2 __far *as2 = addr_2 ();
  struct s1 __far *as3 = addr_3 ();
  struct s2 __far *as4 = addr_4 ();
  unsigned p1, q1, r1, s1, p2, q2, r2, s2, p3, q3, r3, s3, p4, q4, r4, s4;

  printf ("as1 = %#lx, as2 = %#lx\n",
	  (unsigned long) as1, (unsigned long) as2);

  p1 = as1->p;
  p2 = as2->p;
  printf ("p1 = %#x, p2 = %#x\n", p1, p2);
  if (p1 != p2)
    abort ();

  q1 = as1->q;
  q2 = as2->q;
  printf ("q1 = %#x, q2 = %#x\n", q1, q2);
  if (q1 != q2)
    abort ();

  r1 = as1->r;
  r2 = as2->r;
  printf ("r1 = %#x, r2 = %#x\n", r1, r2);
  if (r1 != r2)
    abort ();

  s1 = as1->s;
  s2 = as2->s;
  printf ("s1 = %#x, s2 = %#x\n", s1, s2);
  if (s1 != s2)
    abort ();

  printf ("as3 = %#lx, as4 = %#lx\n",
	  (unsigned long) as3, (unsigned long) as4);

  p3 = as3->p;
  p4 = as4->p;
  printf ("p3 = %#x, p4 = %#x\n", p3, p4);
  if (p3 != p4)
    abort ();

  q3 = as3->q;
  q4 = as4->q;
  printf ("q3 = %#x, q4 = %#x\n", q3, q4);
  if (q3 != q4)
    abort ();

  r3 = as3->r;
  r4 = as4->r;
  printf ("r3 = %#x, r4 = %#x\n", r3, r4);
  if (r3 != r4)
    abort ();

  s3 = as3->s;
  s4 = as4->s;
  printf ("s3 = %#x, s4 = %#x\n", s3, s4);
  if (s3 != s4)
    abort ();

  return 0;
}
