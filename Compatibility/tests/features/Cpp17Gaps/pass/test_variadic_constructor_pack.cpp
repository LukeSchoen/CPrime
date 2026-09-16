// EXPECT_COMPILE_ARGS: -std=c++17
// A constructor template whose parameter pack has no name, reached by
// expanding another function template's body pack around a cast pattern.
#include <type_traits>

template<class T> struct decay_impl { typedef T type; };
template<class T> struct decay_impl<T &> { typedef T type; };

template<class... T> struct Tuple {
  int n;
  template<class... U> explicit Tuple(U &&...) : n(sizeof...(U)) {}
};

template<class... T>
Tuple<typename decay_impl<T>::type...> make(T &&...values) {
  typedef Tuple<typename decay_impl<T>::type...> result_type;
  return result_type(static_cast<T &&>(values)...);
}

int main() {
  Tuple<int, char> value = make(1, 'a');
  return value.n - 2;
}
