template<typename T>
struct Vec2
{
  T x, y;
  Vec2() : x(0), y(0) {}
  Vec2(T ax, T ay) : x(ax), y(ay) {}
};

template<typename T>
struct Vec4
{
  T x, y, z, w;
  Vec4() : x(0), y(0), z(0), w(0) {}
  Vec4(T ax, T ay, T az, T aw) : x(ax), y(ay), z(az), w(aw) {}
};

template<typename T> auto make_vec(T x, T y) { return Vec2<T>(x, y); }
template<typename T> auto make_vec(T x, T y, T z, T w) { return Vec4<T>(x, y, z, w); }

template<typename T> T choose_min(T a, T b) { return a < b ? a : b; }
template<typename T> T choose_max(T a, T b) { return a > b ? a : b; }
template<typename T> auto vec_min(const Vec2<T> &a, const Vec2<T> &b) { return make_vec(choose_min(a.x, b.x), choose_min(a.y, b.y)); }
template<typename T> auto vec_min(const Vec4<T> &a, const Vec4<T> &b) { return make_vec(choose_min(a.x, b.x), choose_min(a.y, b.y), choose_min(a.z, b.z), choose_min(a.w, b.w)); }
template<typename T> auto vec_max(const Vec2<T> &a, const Vec2<T> &b) { return make_vec(choose_max(a.x, b.x), choose_max(a.y, b.y)); }
template<typename T> auto vec_max(const Vec4<T> &a, const Vec4<T> &b) { return make_vec(choose_max(a.x, b.x), choose_max(a.y, b.y), choose_max(a.z, b.z), choose_max(a.w, b.w)); }

int main()
{
  Vec4<float> a(4.0f, 3.0f, 2.0f, 1.0f);
  Vec4<float> b(1.0f, 2.0f, 3.0f, 4.0f);
  Vec4<float> c = vec_min(a, b);
  Vec4<float> d = vec_max(a, b);
  if (c.x != 1.0f || c.y != 2.0f || c.z != 2.0f || c.w != 1.0f)
    return 1;
  if (d.x != 4.0f || d.y != 3.0f || d.z != 3.0f || d.w != 4.0f)
    return 2;
  return 0;
}
