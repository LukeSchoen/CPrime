// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: inline_variables. An inline static data member is a definition
// with a dynamic initializer; reading it must not fault.
struct Counter { static inline int value = 5; };

int main() {
  if (Counter::value != 5) return 1;
  Counter::value = 7;
  return Counter::value - 7;
}
