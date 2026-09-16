// EXPECT_COMPILE_ARGS: -std=c++17
// A fold whose pattern reads constexpr array elements.
constexpr int values[] = {1, 2, 3};

template<int... Is> constexpr int pick() { return (values[Is] + ...); }

static_assert(pick<0, 1, 2>() == 6, "array element reads inside a fold");

int main() { return 0; }
