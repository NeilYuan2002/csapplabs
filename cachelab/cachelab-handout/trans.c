/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  int i,j;
  int block_size =32;
  

  for(i = 0; i < block_size; i += 8){
    for(j = 0; j < block_size; j+= 8){
      for(int cnt = 0; cnt < 8; cnt++,i++){
        int tmp[8];
        tmp[0] = A[i][j];
        tmp[1] = A[i][j+1];
        tmp[2] = A[i][j+2];
        tmp[3] = A[i][j+3];
        tmp[4] = A[i][j+4];
        tmp[5] = A[i][j+5];
        tmp[6] = A[i][j+6];
        tmp[7] = A[i][j+7];

        B[j][i] = tmp[0] ;
        B[j+1][i] = tmp[1] ;
        B[j+2][i] = tmp[2] ;
        B[j+3][i] = tmp[3] ;
        B[j+4][i] = tmp[4] ;
        B[j+5][i] = tmp[5] ;
        B[j+6][i] = tmp[6] ;
        B[j+7][i] = tmp[7] ;
      }
      i -= 8;
    }    
  }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

