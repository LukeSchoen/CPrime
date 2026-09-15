// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: std::tuple is not limited to four elements. Element access,
// tuple_size, the tuple protocol used by structured bindings, make_tuple, tie
// and apply all work at arities above the former limit.
#include <tuple>

struct Pair { int first; int second; };

static int add3(int a, int b, int c) { return a + b + c; }

int main()
{
  std::tuple<int, long, char, double, Pair> five{1, 2L, 'c', 4.0, Pair{5, 6}};
  static_assert(std::tuple_size<std::tuple<int, long, char, double, Pair> >::value == 5,
                "arity");
  static_assert(sizeof(std::tuple_element<4, std::tuple<int, long, char, double, Pair> >::type)
                == sizeof(Pair), "element type");

  auto &[a, b, c, d, e] = five;
  if (a != 1 || b != 2 || c != 'c' || d != 4.0 || e.second != 6) return 1;
  a = 7;
  if (std::get<0>(five) != 7) return 2;
  if (&a != &std::get<0>(five)) return 3;

  auto made = std::make_tuple(1, 2L, 'c', 4.0, Pair{5, 6});
  if (std::get<4>(made).first != 5) return 4;

  std::tuple<int, int, int> three(1, 2, 3);
  if (std::apply(add3, three) != 6) return 5;

  int x = 0;
  std::tuple<int &> alias = std::tie(x);
  std::get<0>(alias) = 9;
  if (x != 9) return 6;

  std::tuple<int, int> pair(1, 2);
  if (std::get<1>(pair) != 2) return 7;
  return 0;
}
