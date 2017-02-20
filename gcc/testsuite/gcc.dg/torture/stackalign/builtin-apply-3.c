/* PR middle-end/12210 */
/* Origin: Ossadchy Yury A. <waspcoder@mail.ru> */

/* This used to fail on i686 because the argument was not copied
   to the right location by __builtin_apply after the direct call.  */

/* { dg-do run } */
/* { dg-require-effective-target untyped_assembly } */


#define INTEGER_ARG  5

#ifdef __ia16__
#define ARGS_SIZE 2
#else
#define ARGS_SIZE 16
#endif

extern void abort(void);

void foo(int arg)
{
  if (arg != INTEGER_ARG)
    abort();
}

void bar(int arg)
{
  foo(arg);
  __builtin_apply(foo, __builtin_apply_args(), ARGS_SIZE);
}

int main(void)
{
  bar(INTEGER_ARG);

  return 0;
}
