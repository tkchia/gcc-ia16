/* { dg-do run } */
/* { dg-options "-Waddress -Os --save-temps" } */

/* First test case for whether GCC warns properly for things like `char
   __far *p, *q;' which are parsed differently by old DOS compilers.  Issue
   found by Bart Oldeman --- https://github.com/tkchia/gcc-ia16/issues/11.  */

void abort (void);

int
main (void)
{
  /* Far pointer declarations.  */
  char __far *p1;	/* { dg-bogus	"will also go in .*__far.* space" } */
  char __far *p2;	/* { dg-bogus	"will also go in .*__far.* space" } */

  /* Generic pointer declarations.  */
  char *p3, *p4;	/* { dg-bogus	"will also go in .*__far.* space" } */

  /* Is p6 a far pointer?  GCC grammar says yes, Watcom says no.  */
  long __far *p5, *p6;	/* { dg-warning	"will also go in .*__far.* space" } */

  /* Watcom does not grok `__far char' at all, so this can have only one
     meaning --- the GCC one.  */
  __far char *p7, *p8;	/* { dg-bogus	"will also go in .*__far.* space" } */

  /* No danger of misinterpretation here.  The `declaration-specifiers' here
     are simply the word `char'.  So p9 is a far pointer to a generic
     pointer, and p10 is a generic pointer to another generic pointer.  */
  char * __far *p9, **p10; /* { dg-bogus "will also go in .*__far.* space" } */

  if (sizeof (p6) != sizeof (long __far *))
    abort ();

  if (sizeof (p9) != sizeof (void __far *))
    abort ();

  if (sizeof (*p9) != sizeof (void *))
    abort ();

  if (sizeof (p10) != sizeof (void *))
    abort ();

  if (sizeof (*p10) != sizeof (void *))
    abort ();

  return 0;
}
