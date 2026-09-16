#include <type_traits>
template<class T> struct Value { static int get() { return sizeof(T); } };
template<class T> int width() {
  return Value<typename std::remove_reference<T>::type>::get();
}
int main() { return width<double>() != sizeof(double); }
