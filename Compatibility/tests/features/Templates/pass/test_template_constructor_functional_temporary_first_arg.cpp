template <typename T>
struct Vec2ForCtor
{
  T x;
  T y;

  Vec2ForCtor() = default;
  Vec2ForCtor(const T &_x, const T &_y) : x(_x), y(_y) {}
};

template <typename T>
struct Vec3ForCtor
{
  T x;
  T y;
  T z;

  Vec3ForCtor() = default;
  Vec3ForCtor(const T &_x, const T &_y, const T &_z) : x(_x), y(_y), z(_z) {}
  Vec3ForCtor(const Vec2ForCtor<T> &_xy, const T &_z);
};

template <typename T>
Vec3ForCtor<T>::Vec3ForCtor(const Vec2ForCtor<T> &_xy, const T &_z)
  : Vec3ForCtor(_xy.x, _xy.y, _z)
{
}

typedef Vec2ForCtor<double> Vec2DForCtor;
typedef Vec3ForCtor<double> Vec3DForCtor;

int main()
{
  Vec3DForCtor pos(Vec2DForCtor(1.0, 2.0), 3.0);
  (void)pos;
  return 0;
}
