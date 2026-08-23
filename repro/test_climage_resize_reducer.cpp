typedef long long i64;
typedef int i32;

template<typename T> T clMax(const T &value) { return value; }
template<typename T, typename... Args> T clMax(const T &value, Args... args)
{
  T maxArgs = clMax(args...);
  return value > maxArgs ? value : maxArgs;
}

template<typename T> struct clVector2
{
  T x, y;
  clVector2() = default;
  clVector2(const T &_x, const T &_y) : x(_x), y(_y) {}
  template<typename U, typename V> explicit clVector2(const U &_x, const V &_y)
    : x(T(_x)), y(T(_y)) {}
};

typedef clVector2<i32> clVec2I;

struct clImage
{
  clVec2I m_size;

  clVec2I Size() const { return m_size; }
  clImage Resize(const clVec2I &newSize, const bool &interpolate = false) const;
  clImage Resize(const i64 &maxSideLen, const bool &interpolate = false) const;
};

clImage clImage::Resize(const clVec2I &newSize, const bool &interpolate) const
{
  return *this;
}

clImage clImage::Resize(const i64 &maxSideLen, const bool &interpolate) const
{
  float longSide = (float)Size().x;
  if (longSide > maxSideLen)
  {
    float sizeMultiplier = longSide / maxSideLen;
    return Resize(clVec2I(clMax(i32(Size().x / sizeMultiplier), 1), clMax(i32(Size().y / sizeMultiplier), 1)), interpolate);
  }
  return *this;
}

int main()
{
  clImage image;
  image.m_size = clVec2I(8, 4);
  return image.Resize(2).Size().x == 2 ? 0 : 1;
}
