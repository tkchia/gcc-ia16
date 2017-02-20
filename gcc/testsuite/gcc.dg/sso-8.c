/* Test support of scalar_storage_order pragma */

/* { dg-do run { target { stdint_types } } } */
/* { dg-options "-fsso-struct=little-endian" } */
/* { dg-require-effective-target int32plus } */

#include <stdint.h>

struct S1
{
  int32_t i;
};

#pragma scalar_storage_order big-endian

struct S2
{
  int32_t i;
};

#pragma scalar_storage_order default

struct S3
{
  int32_t i;
};

struct S1 my_s1 = { 0x12345678 };
struct S2 my_s2 = { 0x12345678 };
struct S3 my_s3 = { 0x12345678 };

unsigned char big_endian_pattern[4] = { 0x12, 0x34, 0x56, 0x78 };
unsigned char little_endian_pattern[4] = { 0x78, 0x56, 0x34, 0x12 };

int main (void)
{
  if (__builtin_memcmp (&my_s1, &little_endian_pattern, 4) != 0)
    __builtin_abort ();

  if (__builtin_memcmp (&my_s2, &big_endian_pattern, 4) != 0)
    __builtin_abort ();

  if (__builtin_memcmp (&my_s3, &little_endian_pattern, 4) != 0)
    __builtin_abort ();

  return 0;
}
