// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 direct-initialization of `auto` with one parenthesized element
// deduces that element's type, exactly like the braced form.

struct Widget
{
  int value;
};

int main()
{
  auto value(1);
  auto scaled(2.5);
  auto widget(Widget{3});
  auto source(4);
  auto &reference(source);
  reference = value;
  return value == 1 && scaled == 2.5 && widget.value == 3 && source == 1 ? 0 : 1;
}
