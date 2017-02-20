/* { dg-lto-do link } */
/* { dg-require-visibility "hidden" } */
/* { dg-require-effective-target fpic } */
/* { dg-require-effective-target shared } */
/* { dg-extra-ld-options { -shared } } */
/* { dg-lto-options { { -fPIC -fvisibility=hidden -flto } } } */
/* { dg-skip-if "-shared not supported" { "ia16-*-*" } }  */

void foo (void *p, void *q, unsigned s)
{
  __builtin_memcpy (p, q, s);
}
