template <typename T>
struct Vec3
{
  T x, y, z;
  Vec3() = default;
  Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
};

template <typename T>
struct Vec4
{
  T x, y, z, w;
  Vec3<T> XYZ() const;
};

template <typename T>
Vec3<T> Vec4<T>::XYZ() const
{
  return Vec3<T>(x, y, z);
}

int main()
{
  Vec4<int> v = { 1, 2, 3, 4 };
  Vec3<int> xyz;
  xyz = v.XYZ();
  return xyz.z == 3 ? 0 : 1;
}
