// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: variant_visit. std::visit, std::monostate, variant_size,
// variant_alternative and the comparison operators are missing, though
// construction and get<> by index and type work.

#include <variant>

int main()
{
  std::variant<int, double> number{21};
  if (std::visit([](auto value) { return static_cast<int>(value); }, number) != 21) return 1;

  std::variant<std::monostate, int> empty;
  if (empty.index() != 0) return 2;

  std::variant<int, double> other{21};
  if (!(number == other)) return 3;

  return std::variant_size<std::variant<int, double>>::value == 2 ? 0 : 4;
}
