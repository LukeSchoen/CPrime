template <typename T>
T clZero()
{
  return T(0);
}

template <typename T>
struct clVector3
{
  T x, y, z;

  clVector3() = default;
  clVector3(const T &_x, const T &_y, const T &_z = clZero<T>())
      : x(_x), y(_y), z(_z)
  {
  }
  template <typename U>
  explicit clVector3(const clVector3<U> &o) : x(T(o.x)), y(T(o.y)), z(T(o.z))
  {
  }
};

typedef clVector3<float> clVec3;

template <typename T>
struct clMatrix4x4
{
  T v[16];

  clMatrix4x4() = default;
  template <typename U>
  explicit clMatrix4x4(const clMatrix4x4<U> &o)
  {
    for (int i = 0; i < 16; ++i)
      v[i] = T(o.v[i]);
  }

  static clMatrix4x4<T> Identity()
  {
    return clMatrix4x4<T>();
  }

  static clMatrix4x4<T> Translation(const clVector3<T> &translation)
  {
    (void)translation;
    return clMatrix4x4<T>();
  }

  template <typename U>
  clMatrix4x4<T> Translated(const clVector3<U> &translation) const
  {
    clMatrix4x4<T> result;
    result.v[12] = T(translation.x);
    result.v[13] = T(translation.y);
    result.v[14] = T(translation.z);
    return result;
  }

  template <typename U>
  clMatrix4x4<T> operator*(const clMatrix4x4<U> &o) const
  {
    clMatrix4x4<T> result;
    result.v[0] = T(v[0] * o.v[0]);
    return result;
  }
};

typedef clMatrix4x4<float> clMat4;

struct DrawState
{
  clVec3 m_pos;

  void Draw(const clMat4 &VP)
  {
    clMat4 MVP = clMat4(VP * clMat4::Translation(clVec3(m_pos.x, m_pos.y, 0)));
    (void)MVP;
  }
};

int main()
{
  DrawState d;
  clMat4 VP;
  d.Draw(VP);
  return 0;
}
