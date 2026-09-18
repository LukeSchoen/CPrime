// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: pack_in_return_template_id. A pack expanded inside a
// template-id in a function's return type. This is what keeps make_tuple, tie
// and apply spelled out to six parameters.
template<class T> struct decay {
  typedef T type;
};

template<class... T> struct Tuple {};

template<class... T>
Tuple<typename decay<T>::type...> wrap(T... values) {
  return Tuple<typename decay<T>::type...>();
}

int main() {
  Tuple<int, char> result = wrap(1, 'a');
  (void)result;
  return 0;
}
