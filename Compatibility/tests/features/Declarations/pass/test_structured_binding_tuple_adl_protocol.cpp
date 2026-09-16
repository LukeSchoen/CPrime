// EXPECT_COMPILE_ARGS: -std=c++17
#include <tuple>
namespace data {
  struct Record { int value; };
  template<unsigned I> int &get(Record &record) { static_assert(I == 0); return record.value; }
  template<unsigned I> int &&get(Record &&record) { static_assert(I == 0); return static_cast<int &&>(record.value); }
  template<unsigned I> const int &get(const Record &record) { static_assert(I == 0); return record.value; }
}
// Ordinary lookup must not make this competing function a candidate.
template<unsigned I> int &get(data::Record &record) = delete;
namespace std {
  template<> struct tuple_size<data::Record> { static constexpr unsigned value = 1; };
  template<> struct tuple_element<0, data::Record> { using type = int; };
  template<> struct tuple_size<const data::Record> { static constexpr unsigned value = 1; };
  template<> struct tuple_element<0, const data::Record> { using type = const int; };
}
template<class A, class B> constexpr bool same = false;
template<class A> constexpr bool same<A, A> = true;
int main() {
  data::Record record = {7};
  auto &[alias] = record;
  static_assert(same<decltype(alias), int>);
  alias = 11;
  auto [copy] = record;
  copy = 13;
  const auto &[constant] = record;
  static_assert(same<decltype(constant), const int>);
  if (&constant != &record.value) return 2;
  return record.value != 11 || copy != 13;
}
