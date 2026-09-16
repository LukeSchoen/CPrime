// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
// Coverage: Clang cxx1z-decomposition.cpp num_elems.
int main() { int values[2] = {1, 2}; auto &[only] = values; return only; }
