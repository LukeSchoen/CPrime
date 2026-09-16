template<class T>
struct ConstantHelper
{
  static inline float Value() { return 1.0f; }
};

template<>
struct ConstantHelper<float>
{
  static inline float Value() { return 2.0f; }
};

template<class T>
inline auto templateConstant()
{
  return ConstantHelper<T>::Value();
}

template<class T>
constexpr auto convertWithTemplateConstant(const T& value)
{
  return (decltype(templateConstant<T>()))(value) * templateConstant<T>();
}

int main()
{
  if (convertWithTemplateConstant(3.0f) != 6.0f)
    return 1;
  return convertWithTemplateConstant(3) == 3.0f ? 0 : 2;
}
