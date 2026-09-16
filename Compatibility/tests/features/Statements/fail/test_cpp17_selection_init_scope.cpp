// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// Coverage: Clang cxx1z-init-statement.cpp: initializer name cannot escape.
int main() { if (int value = 1; value) {} return value; }
