#include <type_traits>
struct Record { int value; int &reference; };
int main() {
  int value = 1;
  const Record record{2, value};
  static_assert(std::is_same<decltype(record.value), int>::value);
  static_assert(std::is_same<decltype(record.reference), int &>::value);
  static_assert(std::is_same<decltype((record.value)), const int &>::value);
  auto closure = [value, &alias = value] {
    static_assert(std::is_same<decltype(value), int>::value);
    static_assert(std::is_same<decltype(alias), int &>::value);
    static_assert(std::is_same<decltype((value)), const int &>::value);
    return value + alias;
  };
  return closure() != 2;
}
