#include <functional>
struct Object { int value; std::function<int()> make(); };
std::function<int()> Object::make() {
  return [&]() { return value + 2; };
}
int main() {
  Object o; o.value = 5;
  auto f = o.make();
  o.value = 9;
  return f() != 11;
}
