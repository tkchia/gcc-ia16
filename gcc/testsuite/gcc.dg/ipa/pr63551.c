/* { dg-do run } */
/* { dg-options "-Os" } */
/* { dg-require-effective-target int32plus } */

union U
{
  unsigned int f0;
  int f1;
};

int a, d;

void
fn1 (union U p)
{
  if (p.f1 <= 0)
    if (a)
      d = 0;
}

void
fn2 ()
{
  d = 0;
  union U b = {
#if __SIZEOF_INT__ == __SIZEOF_SHORT__
    65526U
#else
    4294967286U
#endif
  };
  fn1 (b);
}

int
main ()
{
  fn2 ();
  return 0;
}
