// EXPECT_EXIT: 0

typedef long long i64;

template<i64 N>
constexpr i64 StaticLog2()
{
  return 1 + StaticLog2<(N >> 1)>();
}

template<>
constexpr i64 StaticLog2<1>()
{
  return 0;
}

int main()
{
  return StaticLog2<1>() == 0 ? 0 : 1;
}
