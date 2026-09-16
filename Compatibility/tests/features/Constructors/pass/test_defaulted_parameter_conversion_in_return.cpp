// EXPECT_EXIT: 0
// Returning a value that converts through a constructor with a defaulted
// trailing parameter has to work.  clCoordinateConversion.cpp:28 returns a
// clVec2D expression from a function declared to return clVec3D, which relies
// on `clVector3(const clVector2<T> &xy, const T &z = clZero<T>())`.
struct Vec2
{
  double x, y;
  Vec2() = default;
  Vec2(double px, double py) : x(px), y(py) { }
};

struct Vec3
{
  double x, y, z;
  Vec3() = default;
  Vec3(const Vec2 &xy, const double &zz = 0) : x(xy.x), y(xy.y), z(zz) { }
};

static Vec2 MakeFlat() { return Vec2(1, 2); }
static Vec3 Convert() { return MakeFlat(); }

int main()
{
  Vec3 value = Convert();
  return value.x == 1 && value.y == 2 && value.z == 0 ? 0 : 1;
}
