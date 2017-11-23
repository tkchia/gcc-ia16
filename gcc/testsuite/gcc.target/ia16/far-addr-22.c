/* { dg-do run } */
/* { dg-options "-Waddress -Os --save-temps" } */

/* Second test case for https://github.com/tkchia/gcc-ia16/issues/11.  */

#include <stdarg.h>

struct sigaction;
void abort (void);

int
main (void)
{
  /* Are f2(.) and f3(.) far pointers?  */
  extern unsigned __far
    **f1 (int thang, const struct sigaction *sa, struct sigaction *osa, \
	  void *(* restrict)[8][8], va_list ap, ...),
    *f2 (char),		/* { dg-warning "will also go in .*__far.* space" } */
    *f3 (float);

  if (sizeof (f2 ('x')) != sizeof (unsigned __far *))
    abort ();

  if (sizeof (f3 (3.1415927)) != sizeof (unsigned __far *))
    abort ();

  return 0;
}
