// EXPECT_EXIT: 0

template<typename T>
struct Vec2
{
  T x, y;
  Vec2(T a, T b) : x(a), y(b) {}
  T *Data() { return &x; }
  const T *Data() const { return &x; }
  T &operator[](const long long &index) { return Data()[index]; }
  const T &operator[](const long long &index) const { return Data()[index]; }
};

int main()
{
  Vec2<float> a(1.5f, 2.5f);
  const Vec2<float> &ca = a;
  float x = a[0];
  float y = ca[1];
  return x == 1.5f && y == 2.5f ? 0 : 1;
}
