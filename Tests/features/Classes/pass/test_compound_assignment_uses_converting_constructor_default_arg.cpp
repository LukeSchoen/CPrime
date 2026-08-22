struct Vec2
{
  int x;
  int y;
};

struct Vec3
{
  int x;
  int y;
  int z;

  Vec3() = default;
  Vec3(int vx, int vy, int vz = 0) : x(vx), y(vy), z(vz) {}
  Vec3(const Vec2 &xy, int vz = 0) : x(xy.x), y(xy.y), z(vz) {}

  Vec3 &operator-=(const Vec3 &rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
};

Vec2 operator*(const Vec2 &lhs, const Vec2 &rhs)
{
  Vec2 ret;
  ret.x = lhs.x * rhs.x;
  ret.y = lhs.y * rhs.y;
  return ret;
}

void apply_compound_conversion()
{
  Vec3 value(3, 5, 7);
  Vec2 a;
  Vec2 b;
  a.x = 1;
  a.y = 2;
  b.x = 2;
  b.y = 3;
  value -= a * b;
}

int main()
{
  return 0;
}
