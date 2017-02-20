// { dg-do compile { target { stdint_types } } }
// Ignore warning on some powerpc-linux configurations.
// { dg-prune-output "non-standard ABI extension" }
// { dg-prune-output "changes the ABI" }

#include <stdint.h>

#define vector __attribute__((vector_size(16) ))
vector uint32_t f(int a)
{
  vector uint32_t mask = a ? (vector uint32_t){ 0x80000000, 0x80000000,
0x80000000, 0x80000000 } : (vector uint32_t){0};
  return mask;
}

