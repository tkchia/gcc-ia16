// { dg-do run { target { stdint_types } } }
// { dg-options "-O" }
// { dg-error "limited range of data type" "16-bit target" { target xstormy16-*-* } 0 }
// { dg-error "shift count >=" "16-bit target" { target xstormy16-*-* } 0 }

#include <stdint.h>

int main()
{
  uint32_t x = 1381237248;

  if (sizeof (x) != 4)
    return 0;

  x =
    ({
      uint32_t y = x;
      ({
        uint32_t z = y;
        (uint32_t)
          ((((uint32_t)z & (uint32_t)0x000000ffUL) << 24)
           | (((uint32_t)z & (uint32_t)0x0000ff00UL) << 8)
           | (((uint32_t)z & (uint32_t)0x00ff0000UL) >> 8)
           | (((uint32_t)z & (uint32_t)0xff000000UL) >> 24));
       });
     });
  return x != 152658;
}
