/* { dg-do run } */
/* { dg-options "-Waddress -Os --save-temps" } */

/* Third test case for https://github.com/tkchia/gcc-ia16/issues/11.  */

void abort (void);

int
main (void)
{
  /* We test two things here --- whether the warning is triggered for
     `typedef's, and whether the warning is triggered when we have two
     `__far's in a row just before the declarators begin.  */
  typedef char __far __far
    *t1, *t2;		/* { dg-warning "will also go in .*__far.* space" } */

  if (sizeof (t1) != sizeof (void __far *))
    abort ();

  if (sizeof (t2) != sizeof (void __far *))
    abort ();

  return 0;
}
