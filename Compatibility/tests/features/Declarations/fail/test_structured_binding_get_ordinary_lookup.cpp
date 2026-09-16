// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
#include <tuple>
namespace data { struct Record { int value; }; }
template<unsigned I> int &get(data::Record &record) { return record.value; }
namespace std {
  template<> struct tuple_size<data::Record> { static constexpr unsigned value = 1; };
  template<> struct tuple_element<0, data::Record> { using type = int; };
}
int main() { data::Record record = {3}; auto &[value] = record; }
