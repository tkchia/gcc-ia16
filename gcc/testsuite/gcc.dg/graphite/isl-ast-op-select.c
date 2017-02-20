/* { dg-require-effective-target size32plus } */
/* { dg-require-effective-target int32plus } */
/* { dg-options "-O2 -floop-nest-optimize" } */

#if defined(STACK_SIZE)
#define SIZE 32
#else
#define SIZE 1024
#endif

static void kernel_gemm(int ni, int nj, int nk, double alpha, double beta, double C[SIZE][SIZE], double A[SIZE][SIZE], double B[SIZE][SIZE])
{
 int i, j, k;
 for (i = 0; i < ni; i++)
   for (j = 0; j < nj; j++)
     {
       C[i][j] *= beta;
       for (k = 0; k < nk; ++k)
         C[i][j] += alpha * A[i][k] * B[k][j];
     }
}

void *polybench_alloc_data (int, int);

int main(int argc, char** argv) {
  int ni = SIZE;
  int nj = SIZE;
  int nk = SIZE;
  double alpha;
  double beta;
  double (*C)[SIZE][SIZE];
  C = (double(*)[SIZE][SIZE])polybench_alloc_data ((SIZE) * (SIZE), sizeof(double));
  double (*A)[SIZE][SIZE];
  A = (double(*)[SIZE][SIZE])polybench_alloc_data ((SIZE) * (SIZE), sizeof(double));
  double (*B)[SIZE][SIZE];
  kernel_gemm (ni, nj, nk, alpha, beta, *C, *A, *B);
}
