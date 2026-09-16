#include <type_traits>
int main() {
  auto factory = [](auto value) { return [value] { return value; }; };
  auto result = factory(6);
  auto again = factory(7);
  auto floating = factory(1.5);
  static_assert(std::is_same<decltype(result), decltype(again)>::value);
  static_assert(!std::is_same<decltype(result), decltype(floating)>::value);
  return result() != 6 || again() != 7 || floating() != 1.5;
}
