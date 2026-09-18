// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: cxxplusplus_value. C++17 code gates on __cplusplus, so the
// mode that accepts C++17 language features must report it.
#if __cplusplus < 201703L
#error "__cplusplus below C++17"
#endif

int main() { return 0; }
