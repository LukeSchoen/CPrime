// EXPECT_EXIT: 0

template<typename T>
struct Vec2
{
  T x, y;
  Vec2(T a, T b) : x(a), y(b) {}
  auto Length() const;
};

template<typename T>
auto Vec2<T>::Length() const { return x; }

int main()
{
  Vec2<float> a(1.5f, 2.5f);
  float len = a.Length();
  return len == 1.5f ? 0 : 1;
}
