// EXPECT_EXIT: 0

float clSqrt(float value)
{
  return value;
}

template<typename T>
struct Vec2
{
  T x;
  T y;

  Vec2() : x(0), y(0) {}
  Vec2(T a, T b) : x(a), y(b) {}

  auto Length() const;
};

template<typename T>
auto Vec2<T>::Length() const
{
  return clSqrt(x * x + y * y);
}

int main()
{
  Vec2<float> v(3.0f, 4.0f);
  return v.Length() == 25.0f ? 0 : 1;
}
