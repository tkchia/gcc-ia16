/* { dg-do run } */
/* { dg-options "-O3 -fno-inline --save-temps" } */

/* Test if the compiler successfully constructs the composite type (N1570
   6.2.7) of a function involving the far address space, given several
   declarations of the same function.  */

void abort (void);
int printf (const char *, ...);

int f (unsigned __far matrix[][8], volatile unsigned __far vector[128]);
int f (unsigned __far matrix[16][8], volatile unsigned __far vector[]);

int f (unsigned __far matrix[16][8], volatile unsigned __far vector[128])
{
  unsigned i, j, k;

  if (sizeof (matrix[0]) != 8 * sizeof (unsigned))
    abort ();

  if (sizeof (&matrix[0][0]) != sizeof (void __far *))
    abort ();

  if (sizeof (&matrix[0]) != sizeof (void __far *))
    abort ();

  if (sizeof (vector[0]) != sizeof (unsigned))
    abort ();

  if (sizeof (&vector[0]) != sizeof (void __far *))
    abort ();

  for (i = 0, k = 0; i < 16; ++i)
    {
      for (j = 0; j < 8; ++j, ++k)
	{
	  printf ("%04x ", matrix[i][j]);
	  if (matrix[i][j] != vector[k])
	    abort ();
	}
      printf ("\n");
    }
}

int main (void)
{
  f ((unsigned __far (*)[8]) 0xf000ff00ul,
     (volatile unsigned __far *) 0xfff00000ul);

  return 0;
}
