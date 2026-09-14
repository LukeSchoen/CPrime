template<class T> constexpr int value = 99;
namespace first {
  template<class T> constexpr int value = 1;
  template<class T> constexpr int value<T *> = 2;
  template<class T> constexpr int forwarded = value<T>;
}
namespace second {
  template<class T> constexpr int value = 3;
  template<class T> constexpr int value<T *> = 4;
}
int main() {
  static_assert(first::value<int> == 1);
  static_assert(first::value<int *> == 2);
  static_assert(second::value<int> == 3);
  static_assert(second::value<int *> == 4);
  static_assert(first::forwarded<int *> == 2);
  return &first::value<int> == &second::value<int>;
}
