#include <utility>

struct Input {
  int value;
  explicit Input(int n) : value(n) {}
};

template<class F> struct Storage {
  F function;
  explicit Storage(F input) : function(std::move(input)) {}
};

int main() {
  Input input(42);
  Storage<Input> storage(input);
  return storage.function.value != 42 || input.value != 42;
}
