// PR c++/37932
// { dg-do compile { target c++11 } }

#include <stdint.h>

typedef enum { AA=1, BB=2 } my_enum;

typedef struct { my_enum a:4 ; uint_least32_t b:28; } stru;

void foo (char c, my_enum x, int i)
{
  char arr[2] = {c+'0', 0};	// { dg-error "narrowing" }
  stru s = {x,0};
}
