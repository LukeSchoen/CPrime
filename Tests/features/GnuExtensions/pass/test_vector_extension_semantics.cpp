// GNU vector extension semantics consolidated from the retired external corpus.

typedef int v4si __attribute__((vector_size(16)));

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
  if (int code = check_dependent_vector_size()) { return code; }
  if (int code = check_vector_division()) { return code; }
  return may_alias_member::check();
}
