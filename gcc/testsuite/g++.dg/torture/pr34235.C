/* { dg-do run { target { stdint_types } } } */

#include <stdint.h>

extern "C" void abort (void);
int main()
{
        short x = -1;
        uint_least32_t c = ((uint_least32_t)x) >> 1;
        if (c != 0x7fffffff)
          abort();
        return 0;
}
