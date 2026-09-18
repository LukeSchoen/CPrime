// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_variant_noncopyable. A non-copyable alternative still has
// to support converting assignment and variant move assignment.
#include <variant>

struct Tracked {
  int value;
  Tracked(int v) : value(v) {}
  Tracked(const Tracked &) = delete;
  Tracked &operator=(const Tracked &) = delete;
  Tracked(Tracked &&other) : value(other.value) {}
  Tracked &operator=(Tracked &&other) {
    value = other.value;
    return *this;
  }
};

int main() {
  std::variant<int, Tracked> value;
  value = 1;
  if (value.index() != 0) return 1;

  value = Tracked(2);
  if (value.index() != 1) return 2;
  if (std::get<Tracked>(value).value != 2) return 3;

  std::variant<int, Tracked> other;
  other = Tracked(5);
  value = static_cast<std::variant<int, Tracked> &&>(other);
  return std::get<Tracked>(value).value == 5 ? 0 : 4;
}
