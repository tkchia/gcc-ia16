/* { dg-xfail-if "can cause stack underflow" { nios2-*-* } "*" "" } */
/* { dg-require-effective-target untyped_assembly } */
#define INTEGER_ARG  5

#ifdef __ia16__
#define ARGS_SIZE 2
#else
#define ARGS_SIZE 16
#endif

extern void abort(void);

static void foo(int arg)
{
  if (arg != INTEGER_ARG)
    abort();
}

static void bar(int arg)
{
  foo(arg);
  __builtin_apply(foo, __builtin_apply_args(), ARGS_SIZE);
}

int main(void)
{
  bar(INTEGER_ARG);

  return 0;
}
