typedef int i32;

template<typename T> struct clVector2
{
  T x, y;
  clVector2(const T &_v);
};

template<typename T> clVector2<T>::clVector2(const T &_v) : x(_v), y(_v) {}

typedef clVector2<i32> clVec2I;

int main()
{
  clVec2I v(3);
  return v.x == 3 && v.y == 3 ? 0 : 1;
}
