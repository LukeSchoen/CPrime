template <typename T>
struct ZeroArgTemplateVec
{
  T x;
  T y;

  ZeroArgTemplateVec() = default;
  ZeroArgTemplateVec(const T &_x, const T &_y) : x(_x), y(_y) {}

  template <typename U>
  auto operator*(const U &value) const;
};

template <typename T>
template <typename U>
auto ZeroArgTemplateVec<T>::operator*(const U &value) const
{
  return ZeroArgTemplateVec<T>(x * value, y * value);
}

template <typename T>
ZeroArgTemplateVec<T> MakeZeroArgTemplateVec()
{
  return ZeroArgTemplateVec<T>(3.0f, 6.0f);
}

typedef ZeroArgTemplateVec<float> ZeroArgTemplateVecF;

struct ZeroArgTemplateVec4
{
  ZeroArgTemplateVec4(const ZeroArgTemplateVecF &xy, const float &_z,
                      const float &_w)
      : x(xy.x), y(xy.y), z(_z), w(_w) {}

  float x;
  float y;
  float z;
  float w;
};

float MakeZeroArgTemplateFloat()
{
  return 0.25f;
}

int main()
{
  ZeroArgTemplateVec4 wrapped(
      MakeZeroArgTemplateVec<float>() * MakeZeroArgTemplateFloat(),
      0.0f, 0.0f);
  (void)wrapped;
  return 0;
}
