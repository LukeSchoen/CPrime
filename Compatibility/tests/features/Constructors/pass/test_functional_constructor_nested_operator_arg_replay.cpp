template <typename T>
T zeroValue()
{
  return T(0);
}

template <typename T>
struct Vec3
{
  T x, y, z;

  Vec3() = default;
  Vec3(const T &_x, const T &_y, const T &_z = zeroValue<T>())
      : x(_x), y(_y), z(_z)
  {
  }
  template <typename U>
  explicit Vec3(const Vec3<U> &o) : x(T(o.x)), y(T(o.y)), z(T(o.z))
  {
  }
};

typedef Vec3<float> Vec3f;

template <typename T>
struct Matrix4x4
{
  T v[16];

  Matrix4x4() = default;
  template <typename U>
  explicit Matrix4x4(const Matrix4x4<U> &o)
  {
    for (int i = 0; i < 16; ++i)
      v[i] = T(o.v[i]);
  }

  static Matrix4x4<T> Identity()
  {
    return Matrix4x4<T>();
  }

  static Matrix4x4<T> Translation(const Vec3<T> &translation)
  {
    (void)translation;
    return Matrix4x4<T>();
  }

  template <typename U>
  Matrix4x4<T> Translated(const Vec3<U> &translation) const
  {
    Matrix4x4<T> result;
    result.v[12] = T(translation.x);
    result.v[13] = T(translation.y);
    result.v[14] = T(translation.z);
    return result;
  }

  template <typename U>
  Matrix4x4<T> operator*(const Matrix4x4<U> &o) const
  {
    Matrix4x4<T> result;
    result.v[0] = T(v[0] * o.v[0]);
    return result;
  }
};

typedef Matrix4x4<float> Mat4;

struct DrawState
{
  Vec3f m_pos;

  void Draw(const Mat4 &VP)
  {
    Mat4 MVP = Mat4(VP * Mat4::Translation(Vec3f(m_pos.x, m_pos.y, 0)));
    (void)MVP;
  }
};

int main()
{
  DrawState d;
  Mat4 VP;
  d.Draw(VP);
  return 0;
}
