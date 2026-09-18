template<class T>
struct holder
{
  typedef T value_type;
  static constexpr value_type value[1] = { 0 };
};

template<class T>
constexpr typename holder<T>::value_type holder<T>::value[1];

int main()
{
  return holder<int>::value[0];
}
