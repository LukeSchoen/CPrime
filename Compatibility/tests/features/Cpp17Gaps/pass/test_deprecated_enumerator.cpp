// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: deprecated_enumerator. [[deprecated]] on an enumerator is a
// parse error; the attribute already works on functions and classes.

enum class Mode {
  Legacy [[deprecated]] = 0,
  Current = 1
};

int main()
{
  Mode mode = Mode::Legacy;
  return mode == Mode::Legacy ? 0 : 1;
}
