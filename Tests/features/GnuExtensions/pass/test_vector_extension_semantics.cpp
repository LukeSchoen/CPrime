// GNU vector extension semantics.

typedef int v4si __attribute__((vector_size(16)));
typedef int v2si __attribute__((vector_size(8)));
typedef unsigned char u4qi __attribute__((vector_size(4)));

static v4si global_initializer = {100, 200, 300, 400};

static int check_global_initializer()
{
  return global_initializer[0] == 100 && global_initializer[3] == 400 ? 0 : 1;
}

static int check_element_reference()
{
  __attribute__((__vector_size__(4))) int bytes;
  bytes[0] = 0;
  int &first = bytes[0];
  first = 7;
  return bytes[0] == 7 ? 0 : 1;
}

static int check_boolean_broadcast()
{
  v4si b = {1, 0, -1, 2}, c;
  c = b && 1;
  if (c[0] != -1 || c[1] != 0 || c[2] != -1 || c[3] != -1) { return 1; }
  c = b && 0;
  return (c[0] | c[1] | c[2] | c[3]) == 0 ? 0 : 1;
}

/* A vector comparison is -1 for true and 0 for false per element. */
static int check_vector_comparisons()
{
  v4si a = {1, 2, 3, 4};
  v4si b = {1, 5, 3, 0};
  v4si equal = a == b;
  v4si different = a != b;
  v4si less = a < b;
  v4si greater = a > b;
  v4si at_least = a >= b;
  v4si at_most = a <= b;
  if (equal[0] != -1 || equal[1] != 0 || equal[2] != -1 || equal[3] != 0) { return 1; }
  if (different[0] != 0 || different[1] != -1 || different[2] != 0 || different[3] != -1) { return 2; }
  if (less[0] != 0 || less[1] != -1 || less[2] != 0 || less[3] != 0) { return 3; }
  if (greater[0] != 0 || greater[1] != 0 || greater[2] != 0 || greater[3] != -1) { return 4; }
  if (at_least[0] != -1 || at_least[1] != 0 || at_least[2] != -1 || at_least[3] != -1) { return 5; }
  if (at_most[0] != -1 || at_most[1] != -1 || at_most[2] != -1 || at_most[3] != 0) { return 6; }
  v4si against_zero = a == 3;
  return against_zero[0] == 0 && against_zero[2] == -1 ? 0 : 7;
}

template <int N> struct Dependent
{
  int v __attribute__((vector_size(N * sizeof(int))));

  int last_through_this() { return this->v[N - 1]; }
  int last_direct() { return v[N - 1]; }
};

static int check_dependent_vector_size()
{
  Dependent<4> a = {{0, 1, 2, 3}};
  Dependent<8> b = {{0, 1, 2, 3, 4, 5, 6, 7}};
  if (a.last_through_this() != 3 || a.last_direct() != 3) { return 1; }
  return b.last_through_this() == 7 && b.last_direct() == 7 ? 0 : 1;
}

typedef float v4sf __attribute__((__vector_size__(4 * sizeof(float)), __may_alias__));

static int check_vector_division()
{
  const v4sf a = {12.0f, 12.0f, 12.0f, 12.0f};
  const v4sf b = {2.0f, 2.0f, 2.0f, 2.0f};
  const v4sf c = a / b;
  if (c[0] != 6.0f || c[1] != 6.0f || c[2] != 6.0f || c[3] != 6.0f) { return 1; }

  const v4sf d = {7.0f, 8.0f, 9.0f, 10.0f};
  const v4sf e = {1.0f, 2.0f, 3.0f, 2.0f};
  const v4sf f = d / e;
  return f[0] == 7.0f && f[1] == 4.0f && f[2] == 3.0f && f[3] == 5.0f ? 0 : 1;
}

/* A vector and an integer scalar of the same size convert by reinterpreting
   the same bytes, so `(long long)vector` keeps the object representation. */
static int check_vector_scalar_reinterpretation()
{
  union { long long bits; int elements[2]; } view;
  long long source = 0x0000000100000002LL;
  v2si value = (v2si)source;

  if ((long long)value != source) { return 1; }
  view.bits = source;
  if (value[0] != view.elements[0] || value[1] != view.elements[1]) { return 2; }

  u4qi bytes = {0x11, 0x22, 0x33, 0x44};
  unsigned int packed = (unsigned int)bytes;
  u4qi unpacked = (u4qi)packed;
  if (unpacked[0] != 0x11 || unpacked[1] != 0x22
      || unpacked[2] != 0x33 || unpacked[3] != 0x44) { return 3; }

  /* The original report mixed scalar and vector operands before the cast. */
  long long adjusted = (long long)((0 | value) - ((v2si){} == 0));
  return adjusted == source + 0x0000000100000001LL ? 0 : 4;
}

namespace may_alias_member
{
typedef float __m256 __attribute__((__vector_size__(32), __may_alias__));

struct Holder
{
  __m256 ymm;
  const float &element() const;
};

const float &Holder::element() const
{
  return ymm[1];
}

int check()
{
  return sizeof(Holder) == 32 ? 0 : 1;
}
}

int main()
{
  if (int code = check_global_initializer()) { return code; }
  if (int code = check_element_reference()) { return code; }
  if (int code = check_boolean_broadcast()) { return code; }
  if (int code = check_vector_comparisons()) { return code + 10; }
  if (int code = check_dependent_vector_size()) { return code; }
  if (int code = check_vector_division()) { return code; }
  if (int code = check_vector_scalar_reinterpretation()) { return code; }
  return may_alias_member::check();
}
