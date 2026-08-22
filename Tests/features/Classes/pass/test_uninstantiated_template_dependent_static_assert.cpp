namespace std
{
  template<typename T> struct is_floating_point { enum { value = 0 }; };
  template<> struct is_floating_point<float> { enum { value = 1 }; };
}

#define LOCAL_STATIC_ASSERT(_condition) static_assert(_condition, "dependent assertion")

template<typename T>
T only_for_float(T value)
{
  LOCAL_STATIC_ASSERT(std::is_floating_point<T>::value);
  return value;
}

int main()
{
  return 0;
}
