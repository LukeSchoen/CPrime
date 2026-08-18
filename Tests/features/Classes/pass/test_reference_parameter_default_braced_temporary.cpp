struct Vec3
{
  double x;
  double y;
  double z;
};

struct Camera
{
  Camera(const Vec3 &pos = Vec3{ 0, 0, 0 },
         const Vec3 &rot = Vec3{ 0, 0, 0 },
         double fov = 60);
};

Camera::Camera(const Vec3 &pos, const Vec3 &rot, double fov)
{
  (void)pos;
  (void)rot;
  (void)fov;
}

int main()
{
  Camera camera;
  return 0;
}
