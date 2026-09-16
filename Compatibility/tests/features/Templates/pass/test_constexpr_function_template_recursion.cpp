// A recursion that terminates through an explicit specialization, and one
// that terminates through a conditional, both take part in constant
// evaluation. The static member initializer form is the shape real headers
// use for a bit logarithm.

typedef long long i64;

template<i64 N>
constexpr i64 log2_static()
{
  return 1 + log2_static<(N >> 1)>();
}

template<>
constexpr i64 log2_static<1>()
{
  return 0;
}

struct BitList
{
  typedef unsigned long long BitStorageElementType;
  static const i64 bitStorageElementSize = sizeof(BitStorageElementType) * 8;
  static const i64 bitStorageElementSizeLog2 = log2_static<bitStorageElementSize>();
};

template<i64 N>
constexpr i64 half_log2()
{
  return N > 1 ? 1 + half_log2<(N >> 1)>() : 0;
}

constexpr i64 specialized = log2_static<64>();
constexpr i64 conditional = half_log2<64>();

int main()
{
  return BitList::bitStorageElementSizeLog2 == 6 && specialized == 6
         && conditional == 6 ? 0 : 1;
}
