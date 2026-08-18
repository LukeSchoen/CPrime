namespace std
{
  template<typename T> struct is_signed
  {
    static constexpr bool value = false;
  };
}

template<typename T, bool Signed = false>
struct AbsHelper
{
  static T Abs(T value) { return value; }
};

template<typename T>
struct AbsHelper<T, true>
{
  static T Abs(T value) { return value < 0 ? -value : value; }
};

template<typename T>
T absolute_value(T value)
{
  return AbsHelper<T, std::is_signed<T>::value>::Abs(value);
}

int main()
{
  if (absolute_value(-7) != 7)
    return 1;
  if (absolute_value(9u) != 9u)
    return 2;
  return 0;
}
