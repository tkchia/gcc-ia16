/* PR41779: Wconversion cannot see through real*integer promotions. */
/* { dg-do compile { target { stdint_types } } } */
/* { dg-skip-if "doubles are floats" { "avr-*-*" } { "*" } { "" } } */
/* { dg-options "-std=c99 -Wconversion" { target c } } */
/* { dg-options "-Wconversion" { target c++ } } */
/* { dg-require-effective-target large_double } */

#include <stdint.h>

float f1(float x, unsigned short y)
{
  return x * y;
}

float f2(float x, short y)
{
  return x * y;
}

float f3(float x, char y)
{
  return x * y;
}

float f4(float x, unsigned char y)
{
  return x * y;
}

float f5(float x, int_least32_t y)
{
  return x * y; /* { dg-warning "conversion" } */
}

double c1(float x, unsigned short y, int z)
{
  return z ? x + x : y;
}

double c2(float x, short y, int z)
{
  return z ? x + x : y;
}

double c3(float x, char y, int z)
{
  return z ? x + x : y;
}

double c4(float x, unsigned char y, int z)
{
  return z ? x + x : y;
}

double c5(float x, int_least32_t y, int z)
{
  return z ? x + x : y; /* { dg-warning "conversion" } */
}
