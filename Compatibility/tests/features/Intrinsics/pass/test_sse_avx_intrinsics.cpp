/* x86 intrinsic types and the SSE/AVX/AVX-512 kernels of <immintrin.h>.
   The CNN kernels of the consumer projects use the load/store/multiply-add
   family and the horizontal add, so those are the operations retained here;
   each result is compared against scalar arithmetic. */

#include <immintrin.h>

static int check_types()
{
  if (sizeof(__m64) != 8 || sizeof(__m128) != 16 || sizeof(__m128d) != 16
      || sizeof(__m128i) != 16 || sizeof(__m256) != 32
      || sizeof(__m256d) != 32 || sizeof(__m256i) != 32
      || sizeof(__m512) != 64 || sizeof(__m512d) != 64
      || sizeof(__m512i) != 64)
    return 1;
  return 0;
}

static int check_m128_arithmetic()
{
  __m128 a = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
  __m128 b = _mm_set_ps(8.0f, 6.0f, 4.0f, 2.0f);
  __m128 sum = _mm_add_ps(a, b);
  __m128 difference = _mm_sub_ps(b, a);
  __m128 product = _mm_mul_ps(a, b);
  __m128 quotient = _mm_div_ps(b, a);
  if (sum[0] != 3.0f || sum[3] != 12.0f)
    return 1;
  if (difference[0] != 1.0f || difference[3] != 4.0f)
    return 2;
  if (product[0] != 2.0f || product[1] != 8.0f || product[2] != 18.0f
      || product[3] != 32.0f)
    return 3;
  if (quotient[0] != 2.0f || quotient[3] != 2.0f)
    return 4;
  if (_mm_cvtss_f32(a) != 1.0f)
    return 5;
  return 0;
}

static int check_m128_load_store()
{
  float source[4] = {1.5f, -2.5f, 3.5f, -4.5f};
  float lined[4] = {9.0f, 9.0f, 9.0f, 9.0f};
  __m128 value = _mm_loadu_ps(source);
  _mm_storeu_ps(lined, value);
  if (lined[0] != 1.5f || lined[1] != -2.5f || lined[2] != 3.5f
      || lined[3] != -4.5f)
    return 1;
  __m128 zero = _mm_setzero_ps();
  if (zero[0] != 0.0f || zero[3] != 0.0f)
    return 2;
  __m128 ones = _mm_set1_ps(2.5f);
  if (ones[0] != 2.5f || ones[2] != 2.5f)
    return 3;
  float aligned[4] = {5.0f, 6.0f, 7.0f, 8.0f};
  _mm_store_ps(aligned, _mm_load_ps(aligned));
  if (aligned[3] != 8.0f)
    return 4;
  return 0;
}

static int check_m128_select()
{
  __m128 a = _mm_setr_ps(1.0f, -5.0f, 3.0f, -7.0f);
  __m128 b = _mm_setr_ps(2.0f, -6.0f, -4.0f, 0.0f);
  __m128 maximum = _mm_max_ps(a, b);
  __m128 minimum = _mm_min_ps(a, b);
  if (maximum[0] != 2.0f || maximum[1] != -5.0f || maximum[2] != 3.0f
      || maximum[3] != 0.0f)
    return 1;
  if (minimum[0] != 1.0f || minimum[1] != -6.0f || minimum[2] != -4.0f
      || minimum[3] != -7.0f)
    return 2;
  __m128 mask = _mm_cmplt_ps(a, b);
  /* A comparison intrinsic produces an all-ones lane for true. */
  if (_mm_movemask_ps(mask) != 0x9)
    return 3;
  if (_mm_movemask_ps(_mm_setr_ps(-1.0f, 1.0f, -2.0f, 2.0f)) != 0x5)
    return 4;
  __m128 blended = _mm_blendv_ps(a, b, mask);
  if (blended[0] != 2.0f || blended[3] != 0.0f)
    return 5;
  return 0;
}

static int check_m128_bitwise()
{
  __m128 a = _mm_set1_ps(1.0f);
  __m128 b = _mm_set1_ps(2.0f);
  const unsigned one_bits = 0x3f800000u;
  const unsigned two_bits = 0x40000000u;
  if ((unsigned)_mm_castps_si128(_mm_and_ps(a, a))[0] != one_bits)
    return 1;
  if ((unsigned)_mm_castps_si128(_mm_or_ps(a, b))[0] != (one_bits | two_bits))
    return 2;
  if ((unsigned)_mm_castps_si128(_mm_xor_ps(a, a))[0] != 0u)
    return 3;
  if ((unsigned)_mm_castps_si128(_mm_andnot_ps(a, b))[0]
        != (~one_bits & two_bits))
    return 4;
  if ((unsigned)_mm_castps_si128(_mm_andnot_ps(a, a))[0] != 0u)
    return 5;
  if ((unsigned)_mm_castps_si128(_mm_or_ps(a, a))[0] != one_bits)
    return 6;
  if ((unsigned)_mm_castps_si128(_mm_and_ps(a, b))[0] != 0u)
    return 7;
  if ((unsigned)_mm_castps_si128(_mm_and_ps(a, b))[1] != 0u)
    return 8;
  return 0;
}

static int check_m128_layout()
{
  __m128 a = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
  __m128 b = _mm_setr_ps(5.0f, 6.0f, 7.0f, 8.0f);
  __m128 lo = _mm_unpacklo_ps(a, b);
  __m128 hi = _mm_unpackhi_ps(a, b);
  if (lo[0] != 1.0f || lo[1] != 5.0f || lo[2] != 2.0f || lo[3] != 6.0f)
    return 1;
  if (hi[0] != 3.0f || hi[1] != 7.0f || hi[2] != 4.0f || hi[3] != 8.0f)
    return 2;
  __m128 lowhalves = _mm_movelh_ps(a, b);
  if (lowhalves[0] != 1.0f || lowhalves[3] != 6.0f)
    return 3;
  __m128 highhalves = _mm_movehl_ps(a, b);
  if (highhalves[0] != 7.0f || highhalves[3] != 4.0f)
    return 4;
  __m128 shuffled = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 2, 1, 0));
  if (shuffled[0] != 1.0f || shuffled[1] != 2.0f || shuffled[2] != 7.0f
      || shuffled[3] != 8.0f)
    return 5;
  __m128 permuted = _mm_permute_ps(a, _MM_SHUFFLE(0, 1, 2, 3));
  if (permuted[0] != 4.0f || permuted[3] != 1.0f)
    return 6;
  __m128 summed = _mm_hadd_ps(a, b);
  if (summed[0] != 3.0f || summed[1] != 7.0f || summed[2] != 11.0f
      || summed[3] != 15.0f)
    return 7;
  return 0;
}

static int check_scalar_ss()
{
  __m128 a = _mm_set_ss(6.0f);
  __m128 b = _mm_load_ss(&a[0]);
  __m128 sum = _mm_add_ss(a, b);
  __m128 product = _mm_mul_ss(a, b);
  __m128 difference = _mm_sub_ss(a, b);
  __m128 quotient = _mm_div_ss(a, b);
  float out = 0.0f;
  _mm_store_ss(&out, sum);
  if (out != 12.0f)
    return 1;
  if (product[0] != 36.0f || difference[0] != 0.0f || quotient[0] != 1.0f)
    return 2;
  if (sum[1] != 0.0f)
    return 3;
  if ((unsigned)_mm_castps_si128(_mm_cmplt_ss(a, b))[0] != 0u)
    return 4;
  if ((unsigned)_mm_castps_si128(_mm_cmple_ss(a, b))[0] != 0xffffffffu)
    return 4;
  return 0;
}

static int check_m128d()
{
  __m128d a = _mm_set1_pd(2.0);
  __m128d b = _mm_setzero_pd();
  __m128d sum = _mm_add_pd(a, a);
  __m128d product = _mm_mul_pd(a, sum);
  __m128d quotient = _mm_div_pd(product, a);
  __m128d difference = _mm_sub_pd(product, product);
  if (sum[0] != 4.0 || sum[1] != 4.0 || b[0] != 0.0)
    return 1;
  if (product[0] != 8.0 || quotient[1] != 4.0 || difference[0] != 0.0)
    return 2;
  double lined[2] = {1.0, 2.0};
  __m128d loaded = _mm_loadu_pd(lined);
  _mm_store_pd(lined, _mm_add_pd(loaded, loaded));
  if (lined[0] != 2.0 || lined[1] != 4.0)
    return 3;
  if (_mm_castpd_si128(_mm_cmplt_pd(b, a))[0] != -1)
    return 4;
  if (_mm_castpd_si128(_mm_cmple_pd(a, b))[1] != 0)
    return 4;
  return 0;
}

static int check_m256_arithmetic()
{
  __m256 a = _mm256_setr_ps(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
  __m256 b = _mm256_set1_ps(2.0f);
  __m256 sum = _mm256_add_ps(a, b);
  __m256 difference = _mm256_sub_ps(a, b);
  __m256 product = _mm256_mul_ps(a, b);
  __m256 quotient = _mm256_div_ps(a, b);
  if (sum[0] != 3.0f || sum[7] != 10.0f)
    return 1;
  if (difference[0] != -1.0f || difference[7] != 6.0f)
    return 2;
  if (product[0] != 2.0f || product[7] != 16.0f)
    return 3;
  if (quotient[0] != 0.5f || quotient[7] != 4.0f)
    return 4;
  __m256 maximum = _mm256_max_ps(a, _mm256_set_ps(8.0f, 1.0f, 1.0f, 1.0f,
                                                  1.0f, 1.0f, 1.0f, 1.0f));
  if (maximum[0] != 1.0f || maximum[7] != 8.0f)
    return 5;
  __m256 minimum = _mm256_min_ps(a, b);
  if (minimum[0] != 1.0f || minimum[7] != 2.0f)
    return 6;
  __m256 zero = _mm256_setzero_ps();
  if (zero[0] != 0.0f || zero[7] != 0.0f)
    return 7;
  return 0;
}

static int check_m256_load_store()
{
  float source[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  float lined[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  __m256 value = _mm256_loadu_ps(source);
  _mm256_storeu_ps(lined, value);
  if (lined[0] != 1.0f || lined[7] != 8.0f)
    return 1;
  __m256 aligned = _mm256_load_ps(source);
  _mm256_store_ps(lined, aligned);
  if (lined[4] != 5.0f)
    return 2;
  /* A 256-bit integer round trip through the cast helpers. */
  __m256i integers = _mm256_set1_epi32(-3);
  __m256 as_float = _mm256_castsi256_ps(integers);
  if (_mm256_extract_epi32(_mm256_castps_si256(as_float), 0) != -3)
    return 3;
  return 0;
}

static int check_m256_layout()
{
  __m256 a = _mm256_setr_ps(1, 2, 3, 4, 5, 6, 7, 8);
  __m256 b = _mm256_setr_ps(-1, -2, -3, -4, -5, -6, -7, -8);
  __m256 summed = _mm256_hadd_ps(a, b);
  if (summed[0] != 3.0f || summed[1] != 7.0f || summed[2] != -3.0f
      || summed[3] != -7.0f)
    return 1;
  if (summed[4] != 11.0f || summed[5] != 15.0f || summed[6] != -11.0f
      || summed[7] != -15.0f)
    return 2;
  __m256 lo = _mm256_unpacklo_ps(a, b);
  if (lo[0] != 1.0f || lo[1] != -1.0f || lo[4] != 5.0f || lo[5] != -5.0f)
    return 3;
  __m256 hi = _mm256_unpackhi_ps(a, b);
  if (hi[0] != 3.0f || hi[1] != -3.0f || hi[6] != 8.0f || hi[7] != -8.0f)
    return 4;
  __m256 mask = _mm256_cmpgt_ps(a, _mm256_setzero_ps());
  if (_mm256_movemask_ps(mask) != 0xff)
    return 5;
  /* The bitwise forms operate on lane bits, not float values. */
  if (_mm256_castps_si256(_mm256_and_ps(a, a))[0]
        != _mm256_castps_si256(a)[0])
    return 6;
  if (_mm256_castps_si256(_mm256_or_ps(a, a))[3]
        != _mm256_castps_si256(a)[3])
    return 6;
  if (_mm256_castps_si256(_mm256_xor_ps(a, a))[2] != 0)
    return 6;
  if (_mm256_castps_si256(_mm256_andnot_ps(a, a))[3] != 0)
    return 6;
  if (_mm256_movemask_ps(_mm256_cmpeq_ps(a, a)) != 0xff)
    return 7;
  if (_mm256_movemask_ps(_mm256_cmplt_ps(a, a)) != 0)
    return 7;
  if (_mm256_movemask_ps(_mm256_cmple_ps(a, a)) != 0xff)
    return 7;
  return 0;
}

static int check_m256d()
{
  double lined[4] = {1.0, 2.0, 3.0, 4.0};
  __m256d value = _mm256_load_pd(lined);
  __m256d zero = _mm256_setzero_pd();
  __m256d sum = _mm256_add_pd(value, value);
  __m256d product = _mm256_mul_pd(value, sum);
  _mm256_store_pd(lined, product);
  if (zero[0] != 0.0 || sum[3] != 8.0 || lined[0] != 2.0 || lined[3] != 32.0)
    return 1;
  return 0;
}

static int check_m512()
{
  float source[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  float lined[16];
  __m512 value = _mm512_loadu_ps(source);
  __m512 sum = _mm512_add_ps(value, _mm512_set1_ps(1.0f));
  __m512 product = _mm512_mul_ps(value, value);
  _mm512_storeu_ps(lined, _mm512_sub_ps(sum, product));
  if (lined[0] != 1.0f || lined[15] != -239.0f)
    return 1;
  __m512 maximum = _mm512_max_ps(value, _mm512_setzero_ps());
  if (maximum[0] != 1.0f || maximum[15] != 16.0f)
    return 2;
  __m512 minimum = _mm512_min_ps(value, _mm512_set1_ps(5.0f));
  if (minimum[0] != 1.0f || minimum[15] != 5.0f)
    return 3;
  float reduce = _mm512_reduce_add_ps(value);
  if (reduce != 136.0f)
    return 4;
  if (_mm512_setzero_ps()[0] != 0.0f)
    return 5;
  if (_mm512_castps_si512(_mm512_cmplt_ps(value, value))[0] != 0
      || _mm512_castps_si512(_mm512_cmpgt_ps(value, value))[3] != 0
      || _mm512_castps_si512(_mm512_cmpeq_ps(value, value))[7] != -1)
    return 6;
  if (_mm512_castsi512_ps(_mm512_castps_si512(value))[0] != 1.0f)
    return 7;
  return 0;
}

static int check_integer128()
{
  __m128i zero = _mm_setzero_si128();
  if (_mm_cvtsi128_si32(zero) != 0)
    return 1;
  __m128i a = _mm_set_epi32(4, 3, 2, 1);
  if (_mm_cvtsi128_si32(a) != 1)
    return 2;
  int *lanes = (int *)&a;
  if (lanes[1] != 2 || lanes[3] != 4)
    return 3;
  __m128i broadcast = _mm_set1_epi32(7);
  if (_mm_cvtsi128_si32(broadcast) != 7)
    return 4;
  if (_mm_cvtsi128_si32(_mm_add_epi32(a, broadcast)) != 8)
    return 5;
  if (_mm_cvtsi128_si32(_mm_sub_epi32(broadcast, a)) != 6)
    return 6;
  __m128i sum16 = _mm_add_epi16(_mm_set1_epi16(2), _mm_set1_epi16(3));
  if ((*(short *)&sum16) != 5)
    return 7;
  __m128i sum8 = _mm_sub_epi8(_mm_set1_epi8(9), _mm_set1_epi8(4));
  if ((*(char *)&sum8) != 5)
    return 8;
  __m128i sum64 = _mm_add_epi64(_mm_set1_epi64x(3), _mm_set1_epi64x(4));
  if (sum64[1] != 7)
    return 9;
  __m128i bits = _mm_and_si128(_mm_set1_epi32(6), _mm_set1_epi32(3));
  if (_mm_cvtsi128_si32(bits) != 2)
    return 10;
  if (_mm_cvtsi128_si32(_mm_or_si128(_mm_set1_epi32(1), _mm_set1_epi32(2))) != 3)
    return 11;
  if (_mm_cvtsi128_si32(_mm_xor_si128(_mm_set1_epi32(3), _mm_set1_epi32(1))) != 2)
    return 12;
  if (_mm_cvtsi128_si32(_mm_andnot_si128(_mm_set1_epi32(1),
                                         _mm_set1_epi32(3))) != 2)
    return 13;
  __m128i equal = _mm_cmpeq_epi32(a, a);
  if (_mm_cvtsi128_si32(equal) != -1)
    return 14;
  if (_mm_cvtsi128_si32(_mm_cmpeq_epi16(_mm_set1_epi16(5),
                                        _mm_set1_epi16(5))) != -1)
    return 15;
  if (_mm_cvtsi128_si32(_mm_cmpeq_epi8(_mm_set1_epi8(1),
                                       _mm_set1_epi8(2))) != 0)
    return 16;
  if (_mm_cvtsi128_si32(_mm_slli_epi32(_mm_set1_epi32(3), 2)) != 12)
    return 17;
  if (_mm_cvtsi128_si32(_mm_srli_epi32(_mm_set1_epi32(12), 2)) != 3)
    return 18;
  if (_mm_cvtsi128_si32(_mm_srai_epi32(_mm_set1_epi32(-8), 1)) != -4)
    return 19;
  if (_mm_cvtsi128_si32(_mm_slli_epi64(_mm_set1_epi64x(3), 3)) != 24)
    return 20;
  if (_mm_cvtsi128_si32(_mm_srli_epi64(_mm_set1_epi64x(16), 2)) != 4)
    return 21;
  __m128i lined = _mm_set_epi32(1, 2, 3, 4);
  __m128i stored;
  _mm_storeu_si128(&stored, lined);
  if (_mm_cvtsi128_si32(_mm_loadu_si128(&stored)) != 4)
    return 22;
  _mm_store_si128(&stored, lined);
  if (_mm_cvtsi128_si32(_mm_load_si128(&stored)) != 4)
    return 23;
  if (_mm_cvtsi128_si32(_mm_cvtsi32_si128(11)) != 11)
    return 24;
  __m128i unpacked = _mm_unpacklo_epi32(_mm_set_epi32(0, 0, 0, 2),
                                        _mm_set_epi32(0, 0, 0, 9));
  int *unpacked_lanes = (int *)&unpacked;
  if (unpacked_lanes[0] != 2 || unpacked_lanes[1] != 9)
    return 25;
  __m128i bytes = _mm_unpacklo_epi8(_mm_set1_epi8(1), _mm_set1_epi8(2));
  if ((*(char *)&bytes) != 1 || ((char *)&bytes)[1] != 2)
    return 26;
  __m128i high_bytes = _mm_unpackhi_epi8(_mm_set1_epi8(3), _mm_set1_epi8(4));
  if ((*(char *)&high_bytes) != 3 || ((char *)&high_bytes)[15] != 4)
    return 27;
  __m128i high_words = _mm_unpackhi_epi32(_mm_set_epi32(4, 3, 2, 1),
                                          _mm_set_epi32(8, 7, 6, 5));
  int *high_lanes = (int *)&high_words;
  if (high_lanes[0] != 3 || high_lanes[1] != 7)
    return 28;
  __m128i quarters_lo = _mm_unpacklo_epi64(a, a);
  if (quarters_lo[0] != a[0] || quarters_lo[1] != a[0])
    return 29;
  __m128i quarters_hi = _mm_unpackhi_epi64(a, a);
  if (quarters_hi[0] != a[1] || quarters_hi[1] != a[1])
    return 30;
  return 0;
}

static int check_integer256()
{
  __m256i zero = _mm256_setzero_si256();
  if (zero[0] != 0 || zero[3] != 0)
    return 1;
  __m256i broadcast = _mm256_set1_epi32(9);
  __m256i sum = _mm256_add_epi32(broadcast, broadcast);
  if (_mm256_extract_epi32(sum, 0) != 18 || _mm256_extract_epi32(sum, 7) != 18)
    return 2;
  if (_mm256_extract_epi32(_mm256_sub_epi32(sum, broadcast), 5) != 9)
    return 3;
  if (_mm256_extract_epi32(_mm256_slli_epi32(broadcast, 1), 2) != 18)
    return 4;
  if (_mm256_extract_epi32(_mm256_srli_epi32(broadcast, 0), 6) != 9)
    return 5;
  __m256i anded = _mm256_and_si256(broadcast, _mm256_set1_epi32(3));
  if (_mm256_extract_epi32(anded, 1) != 1)
    return 6;
  if (_mm256_extract_epi32(_mm256_or_si256(_mm256_set1_epi32(1),
                                           _mm256_set1_epi32(2)), 3) != 3)
    return 7;
  if (_mm256_extract_epi32(_mm256_xor_si256(_mm256_set1_epi32(3),
                                            _mm256_set1_epi32(1)), 4) != 2)
    return 8;
  __m256i qwords = _mm256_add_epi64(_mm256_set1_epi32(0), _mm256_set1_epi32(0));
  if (_mm256_extract_epi32(_mm256_sub_epi64(qwords, qwords), 3) != 0)
    return 9;
  __m256i stored;
  _mm256_storeu_si256(&stored, sum);
  if (_mm256_extract_epi32(_mm256_loadu_si256(&stored), 2) != 18)
    return 10;
  if (_mm512_setzero_si512()[0] != 0)
    return 11;
  return 0;
}

static int check_casts()
{
  __m128 floats = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
  __m128i integers = _mm_castps_si128(floats);
  __m128 back = _mm_castsi128_ps(integers);
  if (back[0] != 1.0f || back[3] != 4.0f)
    return 1;
  __m128d doubles = _mm_castps_pd(floats);
  __m128 again = _mm_castpd_ps(doubles);
  if (again[1] != 2.0f || again[3] != 4.0f)
    return 2;
  __m128i double_bits = _mm_castpd_si128(doubles);
  __m128i round_trip = _mm_castps_si128(
                         _mm_castpd_ps(_mm_castsi128_pd(double_bits)));
  if (round_trip[0] != integers[0] || round_trip[1] != integers[1])
    return 3;
  return 0;
}

int main()
{
  if (int code = check_types())
    return code;
  if (int code = check_m128_arithmetic())
    return 10 + code;
  if (int code = check_m128_load_store())
    return 20 + code;
  if (int code = check_m128_select())
    return 30 + code;
  if (int code = check_m128_bitwise())
    return 40 + code;
  if (int code = check_m128_layout())
    return 50 + code;
  if (int code = check_scalar_ss())
    return 60 + code;
  if (int code = check_m128d())
    return 70 + code;
  if (int code = check_m256_arithmetic())
    return 80 + code;
  if (int code = check_m256_load_store())
    return 90 + code;
  if (int code = check_m256_layout())
    return 100 + code;
  if (int code = check_m256d())
    return 110 + code;
  if (int code = check_m512())
    return 120 + code;
  if (int code = check_integer128())
    return 130 + code;
  if (int code = check_integer256())
    return 160 + code;
  return check_casts();
}
