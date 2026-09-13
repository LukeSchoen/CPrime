constexpr int values[] = {1, 3, 5};
static_assert(values[0] == 1 && values[1] == 3 && values[2] == 5,
              "constexpr array elements");

struct triple { int first, second, third; };
constexpr triple nested[2] = {{1, 2, 3}, {4, 5, 6}};
static_assert(nested[0].first == 1 && nested[0].second == 2 && nested[0].third == 3,
              "first aggregate element");
static_assert(nested[1].first == 4 && nested[1].second == 5 && nested[1].third == 6,
              "second aggregate element");

int main() {
  return values[0] != 1 || values[2] != 5
      || nested[1].second != 5 || nested[1].third != 6;
}

// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -std=c++14
