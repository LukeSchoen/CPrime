template <typename T>
struct RangeNestedVec2
{
  T x;
  T y;

  RangeNestedVec2() = default;
  RangeNestedVec2(const T &_x, const T &_y) : x(_x), y(_y) {}

  T &operator[](int index)
  {
    if (index)
      return y;
    return x;
  }
};

template <typename T>
struct RangeNestedList
{
  T data[1];

  int Size() { return 1; }
  T &operator[](int index) { return data[index]; }
};

typedef RangeNestedVec2<int> RangeNestedVec2I;

struct RangeNestedVec4
{
  int x;
  int y;
  int z;
  int w;

  RangeNestedVec2I XY() { return RangeNestedVec2I(x, y); }
};

int main()
{
  RangeNestedList<RangeNestedVec2<RangeNestedVec4>> items;
  items.data[0][0].x = 2;
  items.data[0][0].y = 3;
  items.data[0][1].x = 5;
  items.data[0][1].y = 7;
  int sum = 0;
  for (auto &piece : items)
  {
    RangeNestedVec2I xy = piece[1].XY();
    sum = xy.x + xy.y;
  }
  return 0;
}
