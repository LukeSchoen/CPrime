#ifndef _IMMINTRIN_H
#define _IMMINTRIN_H

/*
 * x86 intrinsic types and the SSE/AVX/AVX-512 kernels of <immintrin.h>.
 *
 * CPC models an intrinsic vector as a GNU vector extension, so the types and
 * the element-wise operations are first class instead of spelled with inline
 * asm.  Only the operations a program actually uses are defined; every
 * intrinsic here is a plain inline function over the vector types, which keeps
 * the header free of clobber declarations the assembler cannot accept
 * (`"xmm0"` is rejected as a clobber register).
 *
 * Floating point lanes follow the SSE/AVX contract: a comparison intrinsic
 * yields an all-ones mask for true, so `a < b` is used directly, and max/min
 * select with that mask through arithmetic instead of a vector `?:`, which the
 * frontend does not have.
 */

/* External projects probe for these macros after including the header.  The
   header is real now, but their optional intrinsic paths were written against
   MSVC headers and stay disabled. */
#undef MA_SUPPORT_AVX
#undef MA_SUPPORT_AVX2

typedef float __m128 __attribute__((vector_size(16)));
typedef double __m128d __attribute__((vector_size(16)));
typedef long long __m128i __attribute__((vector_size(16)));
typedef float __m256 __attribute__((vector_size(32)));
typedef double __m256d __attribute__((vector_size(32)));
typedef long long __m256i __attribute__((vector_size(32)));
typedef float __m512 __attribute__((vector_size(64)));
typedef double __m512d __attribute__((vector_size(64)));
typedef long long __m512i __attribute__((vector_size(64)));

typedef long long __m64 __attribute__((vector_size(8)));

/* Lane views.  Type punning through the address of a vector object is how the
   integer and bitwise intrinsics reach another element width; casting between
   vector types is not implemented. */
typedef int __cpc_v4si __attribute__((vector_size(16)));
typedef unsigned __cpc_v4ui __attribute__((vector_size(16)));
typedef short __cpc_v8hi __attribute__((vector_size(16)));
typedef char __cpc_v16qi __attribute__((vector_size(16)));
typedef long long __cpc_v2di __attribute__((vector_size(16)));
typedef unsigned long long __cpc_v2du __attribute__((vector_size(16)));
typedef int __cpc_v8si __attribute__((vector_size(32)));
typedef unsigned __cpc_v8ui __attribute__((vector_size(32)));
typedef long long __cpc_v4di __attribute__((vector_size(32)));
typedef int __cpc_v16si __attribute__((vector_size(64)));
typedef unsigned __cpc_v16ui __attribute__((vector_size(64)));

#define _MM_SHUFFLE(z, y, x, w) (((z) << 6) | ((y) << 4) | ((x) << 2) | (w))

/* A float comparison yields -1.0f for true and 0.0f for false.  The SSE
   contract for a comparison intrinsic is an all-ones bit mask, so the sign bit
   is broadcast over the lane. */
static __inline __m128 __cpc_mask_ps(__m128 comparison)
{
  __m128 result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&comparison >> 31;
  return result;
}

static __inline __m256 __cpc_mask_256_ps(__m256 comparison)
{
  __m256 result;
  *(__cpc_v8si *)&result = *(__cpc_v8si const *)&comparison >> 31;
  return result;
}

static __inline __m512 __cpc_mask_512_ps(__m512 comparison)
{
  __m512 result;
  *(__cpc_v16si *)&result = *(__cpc_v16si const *)&comparison >> 31;
  return result;
}

static __inline __m128d __cpc_mask_pd(__m128d comparison)
{
  __m128d result;
  *(__cpc_v2di *)&result = *(__cpc_v2di const *)&comparison >> 63;
  return result;
}

static __inline __m256d __cpc_mask_256_pd(__m256d comparison)
{
  __m256d result;
  *(__cpc_v4di *)&result = *(__cpc_v4di const *)&comparison >> 63;
  return result;
}

/* ------------------------------------------------------------------ */
/* 128-bit floating point                                             */
/* ------------------------------------------------------------------ */

static __inline __m128 _mm_setzero_ps(void)
{
  return (__m128){0.0f, 0.0f, 0.0f, 0.0f};
}

static __inline __m128 _mm_set1_ps(float value)
{
  return (__m128){value, value, value, value};
}

static __inline __m128 _mm_set_ps(float e3, float e2, float e1, float e0)
{
  return (__m128){e0, e1, e2, e3};
}

static __inline __m128 _mm_setr_ps(float e0, float e1, float e2, float e3)
{
  return (__m128){e0, e1, e2, e3};
}

static __inline __m128 _mm_load_ps(const float *p)
{
  return *(__m128 const *)p;
}

static __inline __m128 _mm_loadu_ps(const float *p)
{
  return *(__m128 const *)p;
}

static __inline void _mm_store_ps(float *p, __m128 value)
{
  *(__m128 *)p = value;
}

static __inline void _mm_storeu_ps(float *p, __m128 value)
{
  *(__m128 *)p = value;
}

static __inline __m128 _mm_add_ps(__m128 a, __m128 b) { return a + b; }
static __inline __m128 _mm_sub_ps(__m128 a, __m128 b) { return a - b; }
static __inline __m128 _mm_mul_ps(__m128 a, __m128 b) { return a * b; }
static __inline __m128 _mm_div_ps(__m128 a, __m128 b) { return a / b; }

/* max = b + (a - b) * (a > b ? 1 : 0); a comparison yields ~0.0 for true. */
static __inline __m128 _mm_max_ps(__m128 a, __m128 b)
{
  return b + (a - b) * (0.0f - (a > b));
}

static __inline __m128 _mm_min_ps(__m128 a, __m128 b)
{
  return a + (b - a) * (0.0f - (b < a));
}

static __inline __m128 _mm_and_ps(__m128 a, __m128 b)
{
  __m128 result;
  *(__cpc_v4ui *)&result = *(__cpc_v4ui const *)&a & *(__cpc_v4ui const *)&b;
  return result;
}

static __inline __m128 _mm_andnot_ps(__m128 a, __m128 b)
{
  __m128 result;
  *(__cpc_v4ui *)&result = ~(*(__cpc_v4ui const *)&a) & *(__cpc_v4ui const *)&b;
  return result;
}

static __inline __m128 _mm_or_ps(__m128 a, __m128 b)
{
  __m128 result;
  *(__cpc_v4ui *)&result = *(__cpc_v4ui const *)&a | *(__cpc_v4ui const *)&b;
  return result;
}

static __inline __m128 _mm_xor_ps(__m128 a, __m128 b)
{
  __m128 result;
  *(__cpc_v4ui *)&result = *(__cpc_v4ui const *)&a ^ *(__cpc_v4ui const *)&b;
  return result;
}

static __inline __m128 _mm_cmpeq_ps(__m128 a, __m128 b)
{
  return __cpc_mask_ps(a == b);
}

static __inline __m128 _mm_cmplt_ps(__m128 a, __m128 b)
{
  return __cpc_mask_ps(a < b);
}

static __inline __m128 _mm_cmple_ps(__m128 a, __m128 b)
{
  return __cpc_mask_ps(a <= b);
}

static __inline __m128 _mm_cmpgt_ps(__m128 a, __m128 b)
{
  return __cpc_mask_ps(a > b);
}

static __inline __m128 _mm_cmpge_ps(__m128 a, __m128 b)
{
  return __cpc_mask_ps(a >= b);
}

static __inline int _mm_movemask_ps(__m128 a)
{
  const unsigned *bits = (const unsigned *)&a;
  return (int)((bits[0] >> 31) | ((bits[1] >> 31) << 1)
             | ((bits[2] >> 31) << 2) | ((bits[3] >> 31) << 3));
}

static __inline __m128 _mm_hadd_ps(__m128 a, __m128 b)
{
  return (__m128){a[0] + a[1], a[2] + a[3], b[0] + b[1], b[2] + b[3]};
}

static __inline __m128 _mm_unpacklo_ps(__m128 a, __m128 b)
{
  return (__m128){a[0], b[0], a[1], b[1]};
}

static __inline __m128 _mm_unpackhi_ps(__m128 a, __m128 b)
{
  return (__m128){a[2], b[2], a[3], b[3]};
}

static __inline __m128 _mm_movelh_ps(__m128 a, __m128 b)
{
  return (__m128){a[0], a[1], b[0], b[1]};
}

static __inline __m128 _mm_movehl_ps(__m128 a, __m128 b)
{
  return (__m128){b[2], b[3], a[2], a[3]};
}

static __inline __m128 _mm_shuffle_ps(__m128 a, __m128 b, int control)
{
  return (__m128){a[control & 3], a[(control >> 2) & 3],
                  b[(control >> 4) & 3], b[(control >> 6) & 3]};
}

static __inline __m128 _mm_permute_ps(__m128 a, int control)
{
  return (__m128){a[control & 3], a[(control >> 2) & 3],
                  a[(control >> 4) & 3], a[(control >> 6) & 3]};
}

static __inline __m128 _mm_blendv_ps(__m128 a, __m128 b, __m128 mask)
{
  return _mm_or_ps(_mm_and_ps(mask, b), _mm_andnot_ps(mask, a));
}

/* 128-bit doubles. */

static __inline __m128d _mm_setzero_pd(void)
{
  return (__m128d){0.0, 0.0};
}

static __inline __m128d _mm_set1_pd(double value)
{
  return (__m128d){value, value};
}

static __inline __m128d _mm_load_pd(const double *p)
{
  return *(__m128d const *)p;
}

static __inline __m128d _mm_loadu_pd(const double *p)
{
  return *(__m128d const *)p;
}

static __inline void _mm_store_pd(double *p, __m128d value)
{
  *(__m128d *)p = value;
}

static __inline void _mm_storeu_pd(double *p, __m128d value)
{
  *(__m128d *)p = value;
}

static __inline __m128d _mm_add_pd(__m128d a, __m128d b) { return a + b; }
static __inline __m128d _mm_sub_pd(__m128d a, __m128d b) { return a - b; }
static __inline __m128d _mm_mul_pd(__m128d a, __m128d b) { return a * b; }
static __inline __m128d _mm_div_pd(__m128d a, __m128d b) { return a / b; }
static __inline __m128d _mm_cmpeq_pd(__m128d a, __m128d b)
{
  return __cpc_mask_pd(a == b);
}

static __inline __m128d _mm_cmplt_pd(__m128d a, __m128d b)
{
  return __cpc_mask_pd(a < b);
}

static __inline __m128d _mm_cmple_pd(__m128d a, __m128d b)
{
  return __cpc_mask_pd(a <= b);
}

/* Scalar (low lane) forms. */

static __inline __m128 _mm_set_ss(float value)
{
  return (__m128){value, 0.0f, 0.0f, 0.0f};
}

static __inline __m128 _mm_load_ss(const float *p)
{
  return (__m128){p[0], 0.0f, 0.0f, 0.0f};
}

static __inline void _mm_store_ss(float *p, __m128 value)
{
  p[0] = value[0];
}

static __inline float _mm_cvtss_f32(__m128 value)
{
  return value[0];
}

static __inline __m128 _mm_add_ss(__m128 a, __m128 b)
{
  return (__m128){a[0] + b[0], a[1], a[2], a[3]};
}

static __inline __m128 _mm_sub_ss(__m128 a, __m128 b)
{
  return (__m128){a[0] - b[0], a[1], a[2], a[3]};
}

static __inline __m128 _mm_mul_ss(__m128 a, __m128 b)
{
  return (__m128){a[0] * b[0], a[1], a[2], a[3]};
}

static __inline __m128 _mm_div_ss(__m128 a, __m128 b)
{
  return (__m128){a[0] / b[0], a[1], a[2], a[3]};
}

static __inline __m128 _mm_cmplt_ss(__m128 a, __m128 b)
{
  __m128 comparison = (__m128){a[0] < b[0] ? -1.0f : 0.0f, a[1], a[2], a[3]};
  return __cpc_mask_ps(comparison);
}

static __inline __m128 _mm_cmple_ss(__m128 a, __m128 b)
{
  __m128 comparison = (__m128){a[0] <= b[0] ? -1.0f : 0.0f, a[1], a[2], a[3]};
  return __cpc_mask_ps(comparison);
}

/* ------------------------------------------------------------------ */
/* 256-bit floating point                                             */
/* ------------------------------------------------------------------ */

static __inline __m256 _mm256_setzero_ps(void)
{
  return (__m256){0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

static __inline __m256 _mm256_set1_ps(float value)
{
  return (__m256){value, value, value, value, value, value, value, value};
}

static __inline __m256 _mm256_set_ps(float e7, float e6, float e5, float e4,
                                     float e3, float e2, float e1, float e0)
{
  return (__m256){e0, e1, e2, e3, e4, e5, e6, e7};
}

static __inline __m256 _mm256_setr_ps(float e0, float e1, float e2, float e3,
                                      float e4, float e5, float e6, float e7)
{
  return (__m256){e0, e1, e2, e3, e4, e5, e6, e7};
}

static __inline __m256 _mm256_load_ps(const float *p)
{
  return *(__m256 const *)p;
}

static __inline __m256 _mm256_loadu_ps(const float *p)
{
  return *(__m256 const *)p;
}

static __inline void _mm256_store_ps(float *p, __m256 value)
{
  *(__m256 *)p = value;
}

static __inline void _mm256_storeu_ps(float *p, __m256 value)
{
  *(__m256 *)p = value;
}

static __inline __m256 _mm256_add_ps(__m256 a, __m256 b) { return a + b; }
static __inline __m256 _mm256_sub_ps(__m256 a, __m256 b) { return a - b; }
static __inline __m256 _mm256_mul_ps(__m256 a, __m256 b) { return a * b; }
static __inline __m256 _mm256_div_ps(__m256 a, __m256 b) { return a / b; }

static __inline __m256 _mm256_max_ps(__m256 a, __m256 b)
{
  return b + (a - b) * (0.0f - (a > b));
}

static __inline __m256 _mm256_min_ps(__m256 a, __m256 b)
{
  return a + (b - a) * (0.0f - (b < a));
}

static __inline __m256 _mm256_and_ps(__m256 a, __m256 b)
{
  __m256 result;
  *(__cpc_v8ui *)&result = *(__cpc_v8ui const *)&a & *(__cpc_v8ui const *)&b;
  return result;
}

static __inline __m256 _mm256_andnot_ps(__m256 a, __m256 b)
{
  __m256 result;
  *(__cpc_v8ui *)&result = ~(*(__cpc_v8ui const *)&a) & *(__cpc_v8ui const *)&b;
  return result;
}

static __inline __m256 _mm256_or_ps(__m256 a, __m256 b)
{
  __m256 result;
  *(__cpc_v8ui *)&result = *(__cpc_v8ui const *)&a | *(__cpc_v8ui const *)&b;
  return result;
}

static __inline __m256 _mm256_xor_ps(__m256 a, __m256 b)
{
  __m256 result;
  *(__cpc_v8ui *)&result = *(__cpc_v8ui const *)&a ^ *(__cpc_v8ui const *)&b;
  return result;
}

static __inline __m256 _mm256_cmpeq_ps(__m256 a, __m256 b)
{
  return __cpc_mask_256_ps(a == b);
}

static __inline __m256 _mm256_cmplt_ps(__m256 a, __m256 b)
{
  return __cpc_mask_256_ps(a < b);
}

static __inline __m256 _mm256_cmple_ps(__m256 a, __m256 b)
{
  return __cpc_mask_256_ps(a <= b);
}

static __inline __m256 _mm256_cmpgt_ps(__m256 a, __m256 b)
{
  return __cpc_mask_256_ps(a > b);
}

static __inline int _mm256_movemask_ps(__m256 a)
{
  const unsigned *bits = (const unsigned *)&a;
  int result = 0, i;
  for (i = 0; i < 8; ++i)
    result |= (int)((bits[i] >> 31) << i);
  return result;
}

static __inline __m256 _mm256_hadd_ps(__m256 a, __m256 b)
{
  return (__m256){a[0] + a[1], a[2] + a[3], b[0] + b[1], b[2] + b[3],
                  a[4] + a[5], a[6] + a[7], b[4] + b[5], b[6] + b[7]};
}

static __inline __m256 _mm256_unpacklo_ps(__m256 a, __m256 b)
{
  return (__m256){a[0], b[0], a[1], b[1], a[4], b[4], a[5], b[5]};
}

static __inline __m256 _mm256_unpackhi_ps(__m256 a, __m256 b)
{
  return (__m256){a[2], b[2], a[3], b[3], a[6], b[6], a[7], b[7]};
}

static __inline __m256d _mm256_setzero_pd(void)
{
  return (__m256d){0.0, 0.0, 0.0, 0.0};
}

static __inline __m256d _mm256_load_pd(const double *p)
{
  return *(__m256d const *)p;
}

static __inline void _mm256_store_pd(double *p, __m256d value)
{
  *(__m256d *)p = value;
}

static __inline __m256d _mm256_add_pd(__m256d a, __m256d b) { return a + b; }
static __inline __m256d _mm256_mul_pd(__m256d a, __m256d b) { return a * b; }

/* ------------------------------------------------------------------ */
/* 512-bit floating point                                             */
/* ------------------------------------------------------------------ */

static __inline __m512 _mm512_setzero_ps(void)
{
  return (__m512){0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

static __inline __m512 _mm512_set1_ps(float value)
{
  return (__m512){value, value, value, value, value, value, value, value,
                  value, value, value, value, value, value, value, value};
}

static __inline __m512 _mm512_load_ps(const float *p)
{
  return *(__m512 const *)p;
}

static __inline __m512 _mm512_loadu_ps(const float *p)
{
  return *(__m512 const *)p;
}

static __inline void _mm512_store_ps(float *p, __m512 value)
{
  *(__m512 *)p = value;
}

static __inline void _mm512_storeu_ps(float *p, __m512 value)
{
  *(__m512 *)p = value;
}

static __inline __m512 _mm512_add_ps(__m512 a, __m512 b) { return a + b; }
static __inline __m512 _mm512_sub_ps(__m512 a, __m512 b) { return a - b; }
static __inline __m512 _mm512_mul_ps(__m512 a, __m512 b) { return a * b; }
static __inline __m512 _mm512_div_ps(__m512 a, __m512 b) { return a / b; }

static __inline __m512 _mm512_max_ps(__m512 a, __m512 b)
{
  return b + (a - b) * (0.0f - (a > b));
}

static __inline __m512 _mm512_min_ps(__m512 a, __m512 b)
{
  return a + (b - a) * (0.0f - (b < a));
}

static __inline __m512 _mm512_cmplt_ps(__m512 a, __m512 b)
{
  return __cpc_mask_512_ps(a < b);
}

static __inline __m512 _mm512_cmpgt_ps(__m512 a, __m512 b)
{
  return __cpc_mask_512_ps(a > b);
}

static __inline __m512 _mm512_cmpeq_ps(__m512 a, __m512 b)
{
  return __cpc_mask_512_ps(a == b);
}

static __inline float _mm512_reduce_add_ps(__m512 value)
{
  float sum = 0.0f;
  int i;
  for (i = 0; i < 16; ++i)
    sum += value[i];
  return sum;
}

/* ------------------------------------------------------------------ */
/* 128/256-bit integer                                                */
/* ------------------------------------------------------------------ */

static __inline __m128i _mm_setzero_si128(void)
{
  return (__m128i){0, 0};
}

static __inline __m128i _mm_load_si128(const __m128i *p)
{
  return *p;
}

static __inline __m128i _mm_loadu_si128(const __m128i *p)
{
  return *p;
}

static __inline void _mm_store_si128(__m128i *p, __m128i value)
{
  *p = value;
}

static __inline void _mm_storeu_si128(__m128i *p, __m128i value)
{
  *p = value;
}

static __inline __m128i _mm_set_epi32(int e3, int e2, int e1, int e0)
{
  __m128i result;
  *(__cpc_v4si *)&result = (__cpc_v4si){e0, e1, e2, e3};
  return result;
}

static __inline __m128i _mm_set1_epi64x(long long value)
{
  return (__m128i){value, value};
}

static __inline __m128i _mm_set1_epi32(int value)
{
  __m128i result;
  *(__cpc_v4si *)&result = (__cpc_v4si){value, value, value, value};
  return result;
}

static __inline __m128i _mm_set1_epi16(short value)
{
  __m128i result;
  *(__cpc_v8hi *)&result =
    (__cpc_v8hi){value, value, value, value, value, value, value, value};
  return result;
}

static __inline __m128i _mm_set1_epi8(char value)
{
  __m128i result;
  *(__cpc_v16qi *)&result = (__cpc_v16qi){value, value, value, value, value,
    value, value, value, value, value, value, value, value, value, value, value};
  return result;
}

static __inline __m128i _mm_cvtsi32_si128(int value)
{
  return _mm_set_epi32(0, 0, 0, value);
}

static __inline int _mm_cvtsi128_si32(__m128i value)
{
  return (*(const __cpc_v4si *)&value)[0];
}

static __inline __m128i _mm_and_si128(__m128i a, __m128i b)
{
  return a & b;
}

static __inline __m128i _mm_andnot_si128(__m128i a, __m128i b)
{
  return ~a & b;
}

static __inline __m128i _mm_or_si128(__m128i a, __m128i b)
{
  return a | b;
}

static __inline __m128i _mm_xor_si128(__m128i a, __m128i b)
{
  return a ^ b;
}

static __inline __m128i _mm_add_epi32(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&a + *(__cpc_v4si const *)&b;
  return result;
}

static __inline __m128i _mm_sub_epi32(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&a - *(__cpc_v4si const *)&b;
  return result;
}

static __inline __m128i _mm_add_epi16(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v8hi *)&result = *(__cpc_v8hi const *)&a + *(__cpc_v8hi const *)&b;
  return result;
}

static __inline __m128i _mm_sub_epi16(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v8hi *)&result = *(__cpc_v8hi const *)&a - *(__cpc_v8hi const *)&b;
  return result;
}

static __inline __m128i _mm_add_epi8(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v16qi *)&result = *(__cpc_v16qi const *)&a + *(__cpc_v16qi const *)&b;
  return result;
}

static __inline __m128i _mm_sub_epi8(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v16qi *)&result = *(__cpc_v16qi const *)&a - *(__cpc_v16qi const *)&b;
  return result;
}

static __inline __m128i _mm_add_epi64(__m128i a, __m128i b) { return a + b; }
static __inline __m128i _mm_sub_epi64(__m128i a, __m128i b) { return a - b; }

static __inline __m128i _mm_cmpeq_epi32(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&a == *(__cpc_v4si const *)&b;
  return result;
}

static __inline __m128i _mm_cmpeq_epi16(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v8hi *)&result = *(__cpc_v8hi const *)&a == *(__cpc_v8hi const *)&b;
  return result;
}

static __inline __m128i _mm_cmpeq_epi8(__m128i a, __m128i b)
{
  __m128i result;
  *(__cpc_v16qi *)&result = *(__cpc_v16qi const *)&a == *(__cpc_v16qi const *)&b;
  return result;
}

static __inline __m128i _mm_slli_epi32(__m128i a, int count)
{
  __m128i result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&a << count;
  return result;
}

static __inline __m128i _mm_srli_epi32(__m128i a, int count)
{
  __m128i result;
  *(__cpc_v4ui *)&result = *(__cpc_v4ui const *)&a >> count;
  return result;
}

static __inline __m128i _mm_srai_epi32(__m128i a, int count)
{
  __m128i result;
  *(__cpc_v4si *)&result = *(__cpc_v4si const *)&a >> count;
  return result;
}

static __inline __m128i _mm_slli_epi64(__m128i a, int count) { return a << count; }

static __inline __m128i _mm_srli_epi64(__m128i a, int count)
{
  __m128i result;
  *(__cpc_v2du *)&result = *(__cpc_v2du const *)&a >> count;
  return result;
}

static __inline __m128i _mm_unpacklo_epi8(__m128i a, __m128i b)
{
  __m128i result;
  const __cpc_v16qi *x = (const __cpc_v16qi *)&a;
  const __cpc_v16qi *y = (const __cpc_v16qi *)&b;
  *(__cpc_v16qi *)&result = (__cpc_v16qi){(*x)[0], (*y)[0], (*x)[1], (*y)[1],
    (*x)[2], (*y)[2], (*x)[3], (*y)[3], (*x)[4], (*y)[4], (*x)[5], (*y)[5],
    (*x)[6], (*y)[6], (*x)[7], (*y)[7]};
  return result;
}

static __inline __m128i _mm_unpackhi_epi8(__m128i a, __m128i b)
{
  __m128i result;
  const __cpc_v16qi *x = (const __cpc_v16qi *)&a;
  const __cpc_v16qi *y = (const __cpc_v16qi *)&b;
  *(__cpc_v16qi *)&result = (__cpc_v16qi){(*x)[8], (*y)[8], (*x)[9], (*y)[9],
    (*x)[10], (*y)[10], (*x)[11], (*y)[11], (*x)[12], (*y)[12], (*x)[13],
    (*y)[13], (*x)[14], (*y)[14], (*x)[15], (*y)[15]};
  return result;
}

static __inline __m128i _mm_unpacklo_epi32(__m128i a, __m128i b)
{
  __m128i result;
  const __cpc_v4si *x = (const __cpc_v4si *)&a;
  const __cpc_v4si *y = (const __cpc_v4si *)&b;
  *(__cpc_v4si *)&result = (__cpc_v4si){(*x)[0], (*y)[0], (*x)[1], (*y)[1]};
  return result;
}

static __inline __m128i _mm_unpackhi_epi32(__m128i a, __m128i b)
{
  __m128i result;
  const __cpc_v4si *x = (const __cpc_v4si *)&a;
  const __cpc_v4si *y = (const __cpc_v4si *)&b;
  *(__cpc_v4si *)&result = (__cpc_v4si){(*x)[2], (*y)[2], (*x)[3], (*y)[3]};
  return result;
}

static __inline __m128i _mm_unpacklo_epi64(__m128i a, __m128i b)
{
  return (__m128i){a[0], b[0]};
}

static __inline __m128i _mm_unpackhi_epi64(__m128i a, __m128i b)
{
  return (__m128i){a[1], b[1]};
}

static __inline __m256i _mm256_setzero_si256(void)
{
  return (__m256i){0, 0, 0, 0};
}

static __inline __m256i _mm256_loadu_si256(const __m256i *p)
{
  return *p;
}

static __inline void _mm256_storeu_si256(__m256i *p, __m256i value)
{
  *p = value;
}

static __inline __m256i _mm256_set1_epi32(int value)
{
  __m256i result;
  *(__cpc_v8si *)&result = (__cpc_v8si){value, value, value, value,
                                        value, value, value, value};
  return result;
}

static __inline __m256i _mm256_and_si256(__m256i a, __m256i b) { return a & b; }
static __inline __m256i _mm256_or_si256(__m256i a, __m256i b) { return a | b; }
static __inline __m256i _mm256_xor_si256(__m256i a, __m256i b) { return a ^ b; }

static __inline __m256i _mm256_add_epi32(__m256i a, __m256i b)
{
  __m256i result;
  *(__cpc_v8si *)&result = *(__cpc_v8si const *)&a + *(__cpc_v8si const *)&b;
  return result;
}

static __inline __m256i _mm256_sub_epi32(__m256i a, __m256i b)
{
  __m256i result;
  *(__cpc_v8si *)&result = *(__cpc_v8si const *)&a - *(__cpc_v8si const *)&b;
  return result;
}

static __inline __m256i _mm256_add_epi64(__m256i a, __m256i b) { return a + b; }
static __inline __m256i _mm256_sub_epi64(__m256i a, __m256i b) { return a - b; }

static __inline __m256i _mm256_slli_epi32(__m256i a, int count)
{
  __m256i result;
  *(__cpc_v8si *)&result = *(__cpc_v8si const *)&a << count;
  return result;
}

static __inline __m256i _mm256_srli_epi32(__m256i a, int count)
{
  __m256i result;
  *(__cpc_v8ui *)&result = *(__cpc_v8ui const *)&a >> count;
  return result;
}

static __inline int _mm256_extract_epi32(__m256i a, int index)
{
  return (*(const __cpc_v8si *)&a)[index];
}

static __inline __m512i _mm512_setzero_si512(void)
{
  return (__m512i){0, 0, 0, 0, 0, 0, 0, 0};
}

/* ------------------------------------------------------------------ */
/* Reinterpreting casts                                               */
/* ------------------------------------------------------------------ */

static __inline __m128i _mm_castps_si128(__m128 value)
{
  __m128i result;
  *(__m128 *)&result = value;
  return result;
}

static __inline __m128 _mm_castsi128_ps(__m128i value)
{
  __m128 result;
  *(__m128i *)&result = value;
  return result;
}

static __inline __m128d _mm_castps_pd(__m128 value)
{
  __m128d result;
  *(__m128 *)&result = value;
  return result;
}

static __inline __m128 _mm_castpd_ps(__m128d value)
{
  __m128 result;
  *(__m128d *)&result = value;
  return result;
}

static __inline __m128i _mm_castpd_si128(__m128d value)
{
  __m128i result;
  *(__m128d *)&result = value;
  return result;
}

static __inline __m128d _mm_castsi128_pd(__m128i value)
{
  __m128d result;
  *(__m128i *)&result = value;
  return result;
}

static __inline __m256i _mm256_castps_si256(__m256 value)
{
  __m256i result;
  *(__m256 *)&result = value;
  return result;
}

static __inline __m256 _mm256_castsi256_ps(__m256i value)
{
  __m256 result;
  *(__m256i *)&result = value;
  return result;
}

static __inline __m512i _mm512_castps_si512(__m512 value)
{
  __m512i result;
  *(__m512 *)&result = value;
  return result;
}

static __inline __m512 _mm512_castsi512_ps(__m512i value)
{
  __m512 result;
  *(__m512i *)&result = value;
  return result;
}

#endif
