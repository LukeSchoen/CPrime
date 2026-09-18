// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: make_from_tuple. <tuple> has std::apply but not
// std::make_from_tuple.

#include <tuple>

struct Point {
  int x;
  int y;
  Point(int first, int second) : x(first), y(second) {}
};

int main()
{
  Point point = std::make_from_tuple<Point>(std::make_tuple(1, 2));
  return point.x == 1 && point.y == 2 ? 0 : 1;
}
