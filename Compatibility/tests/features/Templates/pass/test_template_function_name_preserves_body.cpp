template<int N> constexpr int Log2Static() { return N + 100; }
template<int N> constexpr int PrefixLog2StaticSuffix() { return N * 3; }
int main() {
  static_assert(Log2Static<8>() == 108);
  return PrefixLog2StaticSuffix<7>() != 21;
}
