template <typename T> T zero_value();

template <typename T>
struct DefaultCtorVec4
{
  T x, y, z, w;

  DefaultCtorVec4() = default;
  DefaultCtorVec4(const T &_x, const T &_y);
  DefaultCtorVec4(const T &_x, const T &_y,
                  const T &_z = zero_value<T>(),
                  const T &_w = zero_value<T>());
};

template <typename T>
DefaultCtorVec4<T>::DefaultCtorVec4(const T &_x, const T &_y)
  : x(_x), y(_y), z(0), w(0)
{
}

template <typename T>
DefaultCtorVec4<T>::DefaultCtorVec4(const T &_x, const T &_y,
                                    const T &_z, const T &_w)
  : x(_x), y(_y), z(_z), w(_w)
{
}

typedef DefaultCtorVec4<float> DefaultCtorVec4F;

int main()
{
  DefaultCtorVec4F a(1.0f, 2.0f);
  DefaultCtorVec4F b(1.0f, 2.0f, 3.0f, 4.0f);
  (void)a;
  (void)b;
  return 0;
}
