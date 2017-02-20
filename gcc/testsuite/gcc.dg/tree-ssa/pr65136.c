/* { dg-do compile { target int32plus } } */
/* { dg-options "-O2 -fdump-rtl-expand-details" } */

#include <stdint.h>

int foo(unsigned int cc )
{

  while ( cc >> 16 )
    {
      cc = (cc & 0xffff) + (cc >> 16);
    }

  return ( (unsigned short)(cc) ) == ((unsigned short)(-1));
}

/* { dg-final { scan-rtl-dump-not "_\[0-9\]* = 1;" "expand" } } */
