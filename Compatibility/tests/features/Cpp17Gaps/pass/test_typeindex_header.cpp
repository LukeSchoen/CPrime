// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_typeindex. <typeindex> is missing from the runtime.
#include <typeindex>
#include <typeinfo>

int main() {
  std::type_index first(typeid(int));
  std::type_index second(typeid(int));
  if (!(first == second)) return 1;
  return first.hash_code() == second.hash_code() ? 0 : 2;
}
