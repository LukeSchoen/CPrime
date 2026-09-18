/* Inline SSE asm next to generated float code in the same function.

   This is a reduced GEMM whose vector loop runs movups/mulps/addps on
   xmm0-xmm2 without a clobber list, and whose leftover columns run scalar
   float arithmetic afterwards.  The shape crashed at N=25 (whose row stride
   is not a multiple of 16 bytes) while N=24 and N=28 were fine, so all three
   are checked against a scalar reference. */

#include <stdlib.h>

#define KP_MA4(b, c, l) __asm__ __volatile__( \
  "movups (%0), %%xmm0\n\t" \
  "movups (%2), %%xmm1\n\t" \
  "mulps %%xmm1, %%xmm0\n\t" \
  "addps (%1), %%xmm0\n\t" \
  "movups %%xmm0, (%1)\n\t" \
  : : "r"(b), "r"(c), "r"(l) : "memory")

static void gemm(int M, int N, int K, const float *A, int lda,
                 const float *B, int ldb, float *C, int ldc)
{
  int n4 = N & ~3;
  for (int i = 0; i < M; i++)
    for (int k = 0; k < K; k++)
    {
      const float *bk = B + (long long)k * ldb;
      float *ci = C + (long long)i * ldc;
      float lanes[4] = { A[i * lda + k], A[i * lda + k],
                         A[i * lda + k], A[i * lda + k] };
      for (int j = 0; j < n4; j += 4) KP_MA4(bk + j, ci + j, lanes);
      for (int j = n4; j < N; j++) ci[j] += A[i * lda + k] * bk[j];
    }
}

/* malloc plus alignment: the asm's movups operands are 16-byte aligned. */
static float *aligned_block(int floats)
{
  unsigned char *raw = (unsigned char *)malloc((unsigned)floats * 4 + 64);
  return (float *)(((unsigned long long)raw + 15) & ~(unsigned long long)15);
}

static int check_gemm(int N)
{
  const int M = 8, K = 27, stride = 64;
  float *A = aligned_block(M * K);
  float *B = aligned_block(K * stride);
  float *C = aligned_block(M * stride);
  float *reference = aligned_block(M * stride);
  int i, j, k;
  if (!A || !B || !C || !reference)
    return 1;
  for (i = 0; i < M * K; i++)
    A[i] = (float)((i * 7) % 13) * 0.25f;
  for (i = 0; i < K * stride; i++)
    B[i] = (float)((i * 5) % 11) * 0.5f;
  for (i = 0; i < M * stride; i++)
    C[i] = reference[i] = 0.0f;

  gemm(M, N, K, A, K, B, stride, C, stride);

  for (i = 0; i < M; i++)
    for (j = 0; j < N; j++)
    {
      float sum = 0.0f;
      for (k = 0; k < K; k++)
        sum += A[i * K + k] * B[k * stride + j];
      reference[i * stride + j] = sum;
    }
  for (i = 0; i < M; i++)
    for (j = 0; j < N; j++)
    {
      float difference = C[i * stride + j] - reference[i * stride + j];
      if (difference < 0.0f)
        difference = -difference;
      if (difference > 0.0001f)
        return 10 + N + j;
    }
  return 0;
}

int main()
{
  if (int code = check_gemm(24))
    return code;
  if (int code = check_gemm(25))
    return code;
  if (int code = check_gemm(28))
    return code;
  return 0;
}
