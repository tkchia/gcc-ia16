/* { dg-require-effective-target int32plus } */
/* { dg-options "-O1 -ffast-math -floop-interchange" } */
/* { dg-require-effective-target stdint_types } */

#include <stdint.h>

int_least32_t adler32(int adler, char *buf, int n)
{
  int sum = 0;
  do {
     adler += buf[0];
     sum += adler;
     adler += buf[1];
     sum += adler;
     adler += buf[2];
     sum += adler;
     adler += buf[3];
     sum += adler;
  } while (--n);
  return adler | ((int_least32_t)sum << 16);
}
