// EXPECT_COMPILE_ARGS: -std=c++17
// A partial specialization whose argument list ends in `>>` must substitute an
// alias template argument before matching.  `add_lvalue_reference` in
// include/runtime/type_traits is written with exactly this spelling, so the
// trait answers with its primary template today.
template<class...> using void_t = void;

template<class T, class = void> struct helper { static const int value = 1; };
template<class T> struct helper<T, void_t<T &>> { static const int value = 2; };

int main() {
  if (helper<int>::value != 2) return 1;
  return 0;
}
