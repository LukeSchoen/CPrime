// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: constexpr_ctor. A user-provided constexpr constructor must be
// usable in a constant expression, not only member calls on the object.
struct Point {
  int x;
  int y;
  constexpr Point(int a, int b) : x(a), y(b) {}
  constexpr int sum() const { return x + y; }
};

constexpr Point origin{0, 0};
constexpr Point unit{1, 2};

static_assert(unit.x == 1 && unit.y == 2, "members initialized by the constructor");
static_assert(unit.sum() == 3, "member call on a constexpr object");
static_assert(Point(3, 4).sum() == 7, "constructor call in a constant expression");

int main() { return unit.sum() - 3; }
