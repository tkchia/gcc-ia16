/* { dg-lto-do run }  */
/* { dg-require-effective-target stdint_types } */
extern "C" { extern void *memcpy (void *, const void *, __SIZE_TYPE__); }

#include <stdint.h>

inline int_least32_t
bci (const float &source)
{
 int_least32_t dest;
 memcpy (&dest, &source, sizeof (dest));
 return dest;
}

inline float
bcf (const int_least32_t &source)
{
 float dest;
 memcpy (&dest, &source, sizeof (dest));
 return dest;
}

float
Foo ()
{
 const int_least32_t foo = bci (0.0f);
 int_least32_t bar = foo;
 const int_least32_t baz = foo & 1;
 if (!baz && (foo & 2))
   bar = 0;
 return bcf (bar);
}

int main ()
{
  if (Foo () != 0.0)
    return 1;
  return 0;
}
