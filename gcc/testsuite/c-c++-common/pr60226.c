/* PR c/60226 */
/* { dg-do compile } */
/* { dg-options "-Wno-c++-compat" { target c } } */
/* { dg-require-effective-target int32plus } */

#ifdef __ia16__
#define SHIFTA 12
#else
#define SHIFTA 28
#define SHIFTB 29
#endif

typedef int __attribute__ ((aligned (1 << SHIFTA))) int28;
int28 foo[4] = {}; /* { dg-error "alignment of array elements is greater than element size|maximum object file alignment" } */
#ifndef __ia16__
typedef int __attribute__ ((aligned (1 << SHIFTB))) int29; /* { dg-error "requested alignment is too large|maximum object file alignment" "" { target { ! ia16-*-* } } } */
#endif

void
f (void)
{
  struct { __attribute__((aligned (1 << SHIFTA))) double a; } x1;
#ifndef __ia16__
  struct { __attribute__((aligned (1 << SHIFTB))) double a; } x2; /* { dg-error "requested alignment is too large" "" { target { ! ia16-*-* } } } */
#endif
}
