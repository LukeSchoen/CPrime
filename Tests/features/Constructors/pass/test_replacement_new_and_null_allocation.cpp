#include <new>
#include <cstdlib>
int allocations, arguments, live;
void *operator new(std::size_t bytes) {
  ++allocations;
  return std::malloc(bytes);
}
void *operator new(std::size_t, const std::nothrow_t&) noexcept { return 0; }
struct Item {
  int value;
  Item(int value) : value(value) { ++live; }
  ~Item() { --live; }
};
struct Aggregate { int value; };
int main() {
  Item *item = new Item(++arguments);
  if (allocations != 1 || arguments != 1 || live != 1 || item->value != 1) return 1;
  delete item;
  Item *missing = new (std::nothrow) Item(++arguments);
  int *scalar = new (std::nothrow) int(++arguments);
  Aggregate *aggregate = new (std::nothrow) Aggregate{++arguments};
  return !missing && !scalar && !aggregate && arguments == 1 && live == 0 ? 0 : 2;
}
