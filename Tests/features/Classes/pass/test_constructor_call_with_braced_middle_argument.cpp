struct Vec2
{
  double x;
  double y;

  Vec2() = default;
};

struct Camera
{
  Camera(int pos, const Vec2 &angles, double fov, double aspect)
  {
    (void)pos;
    (void)angles;
    (void)fov;
    (void)aspect;
  }
};

int main()
{
  Camera camera(1, { 0, 270 }, 45, 1.5);
  return 0;
}
