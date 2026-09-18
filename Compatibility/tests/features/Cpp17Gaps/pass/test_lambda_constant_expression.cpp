// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: constexpr_lambda. A lambda's call operator is implicitly
// constexpr in C++17, but calling one in a constant expression is rejected
// with "constexpr variable initializer is not a constant expression".

int main()
{
  constexpr int answer = [] { return 42; }();
  constexpr int generic = [](auto value) { return static_cast<int>(value); }(7);
  return answer == 42 && generic == 7 ? 0 : 1;
}
