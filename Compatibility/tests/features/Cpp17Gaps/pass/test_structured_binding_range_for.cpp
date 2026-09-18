// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: range_for_structured. Structured bindings must work as the
// declaration of a range-based for loop.
struct Point { int x, y; };

int main() {
  Point points[2] = {{1, 2}, {3, 4}};
  int sum = 0;
  for (auto [x, y] : points) sum += x + y;
  return sum - 10;
}
