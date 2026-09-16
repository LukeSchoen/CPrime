typedef float f32;

template <typename T> struct Vec4 { T x; };

template <typename T> struct Mat4
{
  Vec4<T> v;

  Mat4() { v.x = T(); }
  Mat4(const Vec4<T> &colX, const Vec4<T> &colY);
  Mat4(const T &m0, const T &m1, const T &m2, const T &m3,
       const T &m4, const T &m5, const T &m6, const T &m7,
       const T &m8, const T &m9, const T &m10, const T &m11,
       const T &m12, const T &m13, const T &m14, const T &m15);
  template <typename U> explicit Mat4(const Mat4<U> &o);
  T *Data();
};

typedef Mat4<f32> m4;

void use(const Mat4<float> &m);

int main()
{
  Mat4<float> m;
  use(m);
  return m.v.x != 0.f;
}

void use(const Mat4<float> &m)
{
  (void)m;
}
