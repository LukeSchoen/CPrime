// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: using_declaration_pack. A pack expansion in a
// using-declaration (`using T::f...;`) is a parse error.

struct First {
  int f(int value) { return value + 1; }
};

struct Second {
  int f(double value) { return static_cast<int>(value) + 2; }
};

template <class... Base> struct Combined : Base... {
  using Base::f...;
};

int main()
{
  Combined<First, Second> combined;
  return combined.f(1) == 2 && combined.f(1.5) == 3 ? 0 : 1;
}
