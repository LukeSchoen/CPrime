// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: auto_brace. C++17 deduces `auto x{1}` as int for a single
// element; the declaration is rejected with "unsupported auto declaration".

int main()
{
  auto value{1};
  return value == 1 ? 0 : 1;
}
