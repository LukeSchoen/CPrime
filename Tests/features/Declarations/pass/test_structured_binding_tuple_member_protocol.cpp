// EXPECT_COMPILE_ARGS: -std=c++17
#include <tuple>
struct Record {
  int value;
  template<unsigned I> int &get() & { static_assert(I == 0); return value; }
  template<unsigned I> int &&get() && { static_assert(I == 0); return static_cast<int &&>(value); }
};
namespace std {
  template<> struct tuple_size<Record> { static constexpr unsigned value = 1; };
  template<> struct tuple_element<0, Record> { using type = int; };
}
template<class A, class B> constexpr bool same = false;
template<class A> constexpr bool same<A, A> = true;
int main() {
  Record record = {7};
  auto &[alias] = record;
  static_assert(same<decltype(alias), int>);
  static_assert(same<decltype((alias)), int &>);
  alias = 11;
  auto [copy] = record;
  copy = 13;
  return record.value != 11 || copy != 13;
}
