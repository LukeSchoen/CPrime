template<typename T, bool Signed = false>
struct AbsHelper
{
  static auto Abs(const T &value) { return value; }
};

namespace std
{
  template<typename T>
  struct is_signed
  {
    static constexpr bool value = false;
  };
}

template<typename T>
auto absValue(const T &value)
{
  return AbsHelper<T, std::is_signed<T>::value>::Abs(value);
}

struct Vec3
{
  float x;
  float y;
  float z;
};

int main()
{
  Vec3 value = { 1.0f, 2.0f, 3.0f };
  Vec3 result;
  result = absValue(value);
  return result.x == 1.0f && result.y == 2.0f && result.z == 3.0f ? 0 : 1;
}
