// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: auto_brace. C++17 deduces `auto x{1}` as int for a single
// element; the declaration is rejected with "unsupported auto declaration".

struct Widget
{
  int value;
};

int main()
{
  auto value{1};
  auto scaled{2.5};
  Widget widget{7};
  auto copy{widget};
  return value == 1 && scaled == 2.5 && copy.value == 7 ? 0 : 1;
}
