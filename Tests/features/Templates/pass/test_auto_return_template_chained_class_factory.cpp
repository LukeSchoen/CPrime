template <typename T>
struct ChainedAutoVec
{
  T x;
  T y;

  ChainedAutoVec(const T &_x, const T &_y) : x(_x), y(_y) {}
};

template <typename T>
auto MakeChainedAutoVec(const T &x, const T &y)
{
  return ChainedAutoVec<T>(x, y);
}

template <typename T>
auto TransformChainedAutoVec(const ChainedAutoVec<T> &value, const T &scale)
{
  return MakeChainedAutoVec(value.x * scale, value.y * scale);
}

int main()
{
  ChainedAutoVec<float> value(2.0f, 3.0f);
  value = TransformChainedAutoVec(value, 2.0f);
  return value.x != 4.0f || value.y != 6.0f;
}
