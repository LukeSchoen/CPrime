// EXPECT_EXIT: 0
struct V3D {
  double x;
  double y;
  double z;
};

class Camera {
public:
  V3D position();
};

V3D Camera::position()
{
  V3D value = { 1.0, 2.0, 3.0 };
  return value;
}

struct App {
  Camera camera;
};

int main(void)
{
  App app;
  V3D p = app.camera.position();
  return (p.x == 1.0 && p.y == 2.0 && p.z == 3.0) ? 0 : 1;
}
