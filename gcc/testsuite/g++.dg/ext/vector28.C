/* { dg-do compile { target { stdint_types } } } */

#include <stdint.h>

typedef int32_t veci __attribute__ ((vector_size (4 * sizeof (int32_t))));
typedef float vecf __attribute__ ((vector_size (4 * sizeof (float))));

void f (veci *a, vecf *b, int c)
{
  *a = c || *b;
  *a = *a || c;
}
