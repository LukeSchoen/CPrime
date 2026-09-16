// EXPECT_COMPILE_ARGS: -std=c++17
// A structured binding in a selection-statement initializer crashes the
// compiler with an access violation (exit 0xC0000005). The binding alone and a
// plain selection-statement initializer both compile.

struct Pair {
  int first;
  int second;
};

int main()
{
  if (auto [first, second] = Pair{1, 2}; first + second == 3) return 0;
  return 1;
}
