// EXPECT_COMPILE_ARGS: -std=c++17
// A pack expansion as the first argument of a qualified template-id in a
// parameter type, which `std::integer_sequence<size_t, Indices...>` needs.
#include <stddef.h>

namespace sequence_ns {
  template<class T, T... Values> struct sequence { };
}

template<size_t... Indices>
int count(sequence_ns::sequence<size_t, Indices...>) { return (int)sizeof...(Indices); }

int main() {
  sequence_ns::sequence<size_t, 0, 1> value;
  return count(value) - 2;
}
