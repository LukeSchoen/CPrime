template<class T>
struct RadiansHelper
{
  static inline float Value() { return 0.25f; }
};

template<>
struct RadiansHelper<double>
{
  static inline double Value() { return 0.125; }
};

template<class T>
inline auto radiansPerUnit()
{
  return RadiansHelper<T>::Value();
}

template<class T>
constexpr auto convertToRadians(const T& value)
{
  return (decltype(radiansPerUnit<T>()))(value) * radiansPerUnit<T>();
}

int main()
{
  if (convertToRadians(3) != 0.75f)
    return 1;
  if (convertToRadians(4.0) != 0.5)
    return 2;
  return 0;
}
