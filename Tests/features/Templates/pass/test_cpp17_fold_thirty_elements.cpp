// EXPECT_COMPILE_ARGS: -std=c++17
// A fold over a long pack must not stop at a small expansion count.
template<int... Is> constexpr int sum() { return (Is + ...); }

static_assert(sum<1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, 26, 27, 28, 29, 30>() == 465,
              "thirty-element fold");

int main() { return 0; }
