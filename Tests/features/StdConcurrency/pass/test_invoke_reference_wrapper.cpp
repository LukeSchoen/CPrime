// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <functional>

int increment(int &value, int amount) { value += amount; return value; }
int wrapper_argument(std::reference_wrapper<int> value) { return ++value.get(); }
struct Object {
  int value;
  int add(int amount) { value += amount; return value; }
  int read() const { return value; }
};
struct Derived : Object { int extra; };
struct Pointer {
  Object *value;
  Object &operator*() const { return *value; }
};
struct Callable {
  int calls;
  int operator()(int &value, int amount) { ++calls; return increment(value, amount); }
};

int main() {
  int value = 1;
  if (std::invoke(&increment, std::ref(value), 2) != 3) return 1;
  if (std::invoke(&wrapper_argument, std::ref(value)) != 4) return 2;
  Object object = {5};
  if (std::invoke(&Object::add, object, 1) != 6) return 3;
  if (std::invoke(&Object::add, &object, 2) != 8) return 4;
  if (std::invoke(&Object::add, std::ref(object), 3) != 11) return 5;
  Pointer pointer = {&object};
  if (std::invoke(&Object::add, pointer, 4) != 15) return 6;
  std::invoke(&Object::value, std::ref(object)) = 21;
  if (std::invoke(&Object::read, std::cref(object)) != 21) return 7;
  Derived derived;
  derived.value = 2;
  if (std::invoke(&Object::add, derived, 3) != 5) return 8;
  Callable callable = {0};
  auto reference = std::ref(callable);
  if (reference(value, 5) != 9 || callable.calls != 1) return 9;
  auto function = std::ref(increment);
  if (function(value, 2) != 11) return 10;
  return value != 11 || object.value != 21;
}
