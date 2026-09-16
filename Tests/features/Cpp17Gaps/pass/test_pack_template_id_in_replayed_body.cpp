// EXPECT_COMPILE_ARGS: -std=c++17
// A template-id that spells a pack inside a replaying template body (here the
// function's parameter type spells `Box<Types...>`) must resolve to the same
// specialization the direct use does, rather than to one that kept only the
// pack's first argument.
#include <stddef.h>

template<class... Types> struct Box { };

template<size_t Index> int element_at() { return (int)Index; }

template<class T, class... Types> struct Index;
template<class T, class... Rest>
struct Index<T, T, Rest...> { static const size_t value = 0; };
template<class T, class First, class... Rest>
struct Index<T, First, Rest...> { static const size_t value = 1 + Index<T, Rest...>::value; };

template<class T, class... Types>
int position(const Box<Types...> &) { return element_at<Index<T, Types...>::value>(); }

int main() {
  Box<int, double> value;
  if (position<double>(value) != 1) return 1;
  if (Index<double, int, double>::value != 1) return 2;
  return 0;
}
