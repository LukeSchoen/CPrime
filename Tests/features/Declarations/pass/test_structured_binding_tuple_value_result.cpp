// EXPECT_COMPILE_ARGS: -std=c++17
#include <tuple>
struct Record {
  short value;
  template<unsigned I> short get() const { static_assert(I == 0); return value; }
};
namespace std {
  template<> struct tuple_size<Record> { static constexpr unsigned value = 1; };
  template<> struct tuple_element<0, Record> { using type = int; };
}
template<class A, class B> constexpr bool same = false;
template<class A> constexpr bool same<A, A> = true;
int main() {
  Record record = {17};
  auto &[value] = record;
  static_assert(same<decltype(value), int>);
  value = 257;
  return value != 257 || record.value != 17;
}
