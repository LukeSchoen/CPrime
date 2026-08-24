template <typename T>
struct ScalarOperatorVec
{
  T x;
  T y;

  ScalarOperatorVec() = default;
  ScalarOperatorVec(const T &_x, const T &_y) : x(_x), y(_y) {}

  template <typename U>
  auto operator*(const U &value) const
  {
    return ScalarOperatorVec<T>(x * (T)value, y * (T)value);
  }
};

typedef ScalarOperatorVec<float> ScalarOperatorVecF;

struct ScalarOperatorVec4
{
  float x;
  float y;
  float z;
  float w;

  ScalarOperatorVec4() = default;
  ScalarOperatorVec4(const ScalarOperatorVecF &xy, const float &_z,
                     const float &_w)
      : x(xy.x), y(xy.y), z(_z), w(_w) {}
};

int main()
{
  ScalarOperatorVecF vec = { 1.0f, 2.0f };
  ScalarOperatorVec4 wrapped(vec * 0.5f, 0.0f, 0.0f);
  return wrapped.x == 0.5f && wrapped.y == 1.0f
           && wrapped.z == 0.0f && wrapped.w == 0.0f ? 0 : 1;
}
