// EXPECT_EXIT: 0

template<typename T>
struct Vec2
{
  T x, y;
  Vec2(T a, T b) : x(a), y(b) {}
  static Vec2 Zero();
};

template<typename T>
Vec2<T> Vec2<T>::Zero()
{
  Vec2<T> r(0, 0);
  return r;
}

int main()
{
  Vec2<float> z = Vec2<float>::Zero();
  return z.x == 0.0f && z.y == 0.0f ? 0 : 1;
}
