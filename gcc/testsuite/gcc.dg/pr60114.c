/* PR c/60114 */
/* { dg-do compile { target { stdint_types } } } */
/* { dg-options "-Wconversion" } */
/* { dg-require-effective-target int32plus } */

#include <stdint.h>

struct S { int32_t n, u[2]; };
const signed char z[] = {
  [0] = 0x100, /* { dg-warning "9:overflow in implicit constant conversion" } */
  [2] = 0x101, /* { dg-warning "9:overflow in implicit constant conversion" } */
};
int32_t A[] = {
            0, 0x80000000, /* { dg-warning "16:conversion of unsigned constant value to negative integer" } */
            0xA, 0x80000000, /* { dg-warning "18:conversion of unsigned constant value to negative integer" } */
            0xA, 0xA, 0x80000000 /* { dg-warning "23:conversion of unsigned constant value to negative integer" } */
          };
int32_t *p = (int32_t []) { 0x80000000 }; /* { dg-warning "29:conversion of unsigned constant value to negative integer" } */
union { int32_t k; } u = { .k = 0x80000000 }; /* { dg-warning "33:conversion of unsigned constant value to negative integer" } */
typedef int32_t H[];
void
foo (void)
{
  signed char a[][3] = { { 0x100, /* { dg-warning "28:overflow in implicit constant conversion" } */
                    1, 0x100 }, /* { dg-warning "24:overflow in implicit constant conversion" } */
                  { '\0', 0x100, '\0' } /* { dg-warning "27:overflow in implicit constant conversion" } */
                };
  (const signed char []) { 0x100 }; /* { dg-warning "28:overflow in implicit constant conversion" } */
  (const float []) { 1e0, 1e1, 1e100 }; /* { dg-warning "32:conversion" } */
  struct S s1 = { 0x80000000 }; /* { dg-warning "19:conversion of unsigned constant value to negative integer" } */
  struct S s2 = { .n = 0x80000000 }; /* { dg-warning "24:conversion of unsigned constant value to negative integer" } */
  struct S s3 = { .u[1] = 0x80000000 }; /* { dg-warning "27:conversion of unsigned constant value to negative integer" } */
  H h = { 1, 2, 0x80000000 }; /* { dg-warning "17:conversion of unsigned constant value to negative integer" } */
}
