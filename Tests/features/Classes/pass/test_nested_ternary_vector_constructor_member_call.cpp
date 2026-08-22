template<typename T>
struct Vec2
{
  T x, y;
  Vec2() : x(0), y(0) {}
  Vec2(T ax, T ay) : x(ax), y(ay) {}
  template<typename U> auto operator*(const Vec2<U> &rhs) const;
  template<typename U> auto operator*(const U &rhs) const;
};

template<typename T>
struct Vec3
{
  T x, y, z;
  Vec3() : x(0), y(0), z(0) {}
  Vec3(T ax, T ay, T az = T()) : x(ax), y(ay), z(az) {}
  Vec3(const Vec2<T> &xy, T az = T()) : x(xy.x), y(xy.y), z(az) {}
  template<typename U> auto operator-(const Vec3<U> &rhs) const;
};

template<typename T> auto make_vec(T x, T y) { return Vec2<T>(x, y); }
template<typename T> auto make_vec(T x, T y, T z) { return Vec3<T>(x, y, z); }

template<typename T> template<typename U>
auto Vec2<T>::operator*(const Vec2<U> &rhs) const
{
  return make_vec(x * rhs.x, y * rhs.y);
}

template<typename T> template<typename U>
auto Vec2<T>::operator*(const U &rhs) const
{
  return make_vec(x * rhs, y * rhs);
}

template<typename T> template<typename U>
auto Vec3<T>::operator-(const Vec3<U> &rhs) const
{
  return make_vec(x - rhs.x, y - rhs.y, z - rhs.z);
}

class CameraBase
{
public:
  CameraBase() : rotation(1.0, 2.0, 3.0) {}
  Vec3<double> GetRotation() const { return rotation; }
  void SetRotation(const Vec3<double> &value) { rotation = value; }
  Vec3<double> rotation;
};

class Camera : public CameraBase
{
public:
  void Apply(bool flipped, double turn, double dt);
};

void Camera::Apply(bool flipped, double turn, double dt)
{
  Vec2<double> axis(2.0, 3.0);
  SetRotation(GetRotation() - Vec3<double>(axis * (flipped ? Vec2<double>(-1.0, 1.0) : Vec2<double>(1.0, 1.0)) * 10.0 * turn * dt, 0.0));
}

int main()
{
  Camera camera;
  camera.Apply(false, 1.0, 0.5);
  return 0;
}
