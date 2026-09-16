template <typename T>
struct MemberVec2
{
  T x;
  T y;
};

template <typename T>
struct MemberVec4
{
  T x;
  T y;
  T z;
  T w;

  MemberVec4(const T &_x, const T &_y, const T &_z = 0, const T &_w = 0)
      : x(_x), y(_y), z(_z), w(_w) {}
  MemberVec4(const MemberVec2<T> &_xy, const T &_z = 0, const T &_w = 0)
      : x(_xy.x), y(_xy.y), z(_z), w(_w) {}

  MemberVec2<T> XY() const
  {
    MemberVec2<T> ret = { x, y };
    return ret;
  }
};

typedef MemberVec4<float> MemberVec4F;

int main()
{
  MemberVec4F source(1.0f, 2.0f, 3.0f, 4.0f);
  MemberVec4F value(source.XY(), 0.0f, 0.0f);
  (void)value;
  return 0;
}
