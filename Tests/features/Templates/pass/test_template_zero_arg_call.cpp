// EXPECT_EXIT: 0

template<typename T> T one() { return 1; }
template<typename T> T zero() { return T(); }

int main()
{
  float f = one<float>();
  int i = one<int>();
  int z = zero<int>();
  return f == 1.0f && i == 1 && z == 0 ? 0 : 1;
}
