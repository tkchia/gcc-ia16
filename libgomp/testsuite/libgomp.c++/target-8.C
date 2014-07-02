// { dg-do run }
// { dg-additional-options "-flto" }
// { dg-require-effective-target target_device }

#include <omp.h>

const int ROWS = 5;
const int COLS = 5;

extern "C" void abort (void);

void init (int Q[][COLS], const int rows, const int cols)
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      Q[i][j] = (i+1) * 100 + (j+1);
}

void check (int a[][COLS], int b[][COLS], const int rows, const int cols)
{
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      if (a[i][j] != b[i][j])
	abort ();
}

void gramSchmidt (int Q[][COLS], const int rows, const int cols)
{
  for (int k = 0; k < cols; k++)
    {
      int tmp = 0;

      for (int i = 0; i < rows; i++)
	tmp += (Q[i][k] * Q[i][k]);

      for (int i = 0; i < rows; i++)
	Q[i][k] *= tmp;
    }
}


void gramSchmidt_1 (int Q[][COLS], const int rows, const int cols)
{
  #pragma omp target data map(Q[0:rows][0:cols])
    for (int k = 0; k < cols; k++)
      {
	int tmp = 0;

	#pragma omp target
	  #pragma omp parallel for reduction(+:tmp)
	    for (int i = 0; i < rows; i++)
	      tmp += (Q[i][k] * Q[i][k]);

	#pragma omp target
	  #pragma omp parallel for
	    for (int i = 0; i < rows; i++)
	      Q[i][k] *= tmp;
      }
}

int main ()
{
  int (*Q)[COLS] = (int(*)[COLS]) new int[ROWS * COLS];
  int (*Q1)[COLS] = (int(*)[COLS]) new int[ROWS * COLS];

  init (Q, ROWS, COLS);
  init (Q1, ROWS, COLS);

  gramSchmidt (Q, ROWS, COLS);
  gramSchmidt_1 (Q1, ROWS, COLS);

  check (Q, Q1, ROWS, COLS);

  delete [] Q;
  delete [] Q1;

  return 0;
}
