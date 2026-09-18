// EXPECT_EXIT: 0
// A constructor argument written as a braced-init-list for a parameter whose
// class constructor has defaulted parameters has to select that constructor.
// `Quad({ a, b, c, d })` against `Quad(const Vec4 &)` is the shape.
struct Vec3
{
  float x, y, z;
};

struct Vec4
{
  Vec3 x, y, z, w;
  Vec4(const Vec3 &a, const Vec3 &b, const Vec3 &c = Vec3(), const Vec3 &d = Vec3())
    : x(a), y(b), z(c), w(d) { }
};

struct Quad
{
  Vec4 verts;
  Quad(const Vec4 &corners) : verts(corners) { }
};

static Quad BuildQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d)
{
  return Quad({ a, b, c, d });
}

int main()
{
  Vec3 a = { 1, 2, 3 }, b = { 4, 5, 6 }, c = { 7, 8, 9 }, d = { 10, 11, 12 };
  Quad quad = BuildQuad(a, b, c, d);
  if (quad.verts.x.x != 1 || quad.verts.w.z != 12) return 1;
  return 0;
}
