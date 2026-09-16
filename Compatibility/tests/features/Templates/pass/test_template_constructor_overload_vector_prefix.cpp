template <typename T>
struct PrefixVec2
{
  T x;
  T y;
};

template <typename T>
struct PrefixVec4
{
  T x;
  T y;
  T z;
  T w;

  PrefixVec4(const T &_x, const T &_y, const T &_z, const T &_w)
      : x(_x), y(_y), z(_z), w(_w) {}
  PrefixVec4(const PrefixVec2<T> &_xy, const T &_z, const T &_w)
      : x(_xy.x), y(_xy.y), z(_z), w(_w) {}
};

typedef PrefixVec2<float> PrefixVec2F;
typedef PrefixVec4<float> PrefixVec4F;

int main()
{
  PrefixVec2F xy = { 1.0f, 2.0f };
  PrefixVec4F value(xy, 3.0f, 4.0f);
  (void)value;
  return 0;
}
