struct Vec3
{
  double x;
};

struct Vec4
{
  double x;

  Vec3 XYZ() const
  {
    Vec3 v;
    v.x = x;
    return v;
  }
};

Vec4 operator*(const Vec4 &a, const Vec4 &b)
{
  Vec4 r;
  r.x = a.x + b.x;
  return r;
}

int main()
{
  Vec4 a;
  Vec4 b;
  a.x = 2.0;
  b.x = 5.0;
  Vec3 c = (a * b).XYZ();
  return c.x == 7.0 ? 0 : 1;
}
