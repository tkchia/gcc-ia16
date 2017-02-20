/* { dg-require-effective-target size32plus } */
int foo (void);
void bar (void);

#if defined (STACK_SIZE) && STACK_SIZE <= 50000
#define LARGE 100
#define SMALL 50
#else
#define LARGE 200
#define SMALL 100
#endif

int toto()
{
  /* Scop 1. */
  int i, j, k;
  int a[LARGE + 1][SMALL];
  int b[SMALL];
  int N = foo ();

  for (i = 0; i < N+ SMALL; i++)
    for (j = 0; j < LARGE; j++)
      a[j][i] = a[j+1][10] + 2;

  return a[3][5] + b[1];
  /* End scop 1. */
}

/* { dg-final { scan-tree-dump-times "number of SCoPs: 1" 1 "graphite"} } */

