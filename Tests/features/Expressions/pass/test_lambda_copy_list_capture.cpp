#include <initializer_list>
#include <type_traits>
int calls;
int next_value() { return ++calls; }
int main() {
  auto closure = [values = {0, next_value(), next_value(), next_value()}] {
    static_assert(std::is_same<decltype(values), std::initializer_list<int>>::value, "deduced list type");
    int total = 0;
    for (int value : values) total += value;
    return total;
  };
  return calls != 3 || closure() != 6 || closure() != 6;
}
