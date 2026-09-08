#include <typeinfo>
#include <string.h>
struct Left { virtual ~Left() {} int left; };
struct Right { virtual ~Right() {} int right; };
struct Derived : Left, Right {};
struct Plain {};
int effects;
int bump() { ++effects; return 4; }
int main() {
  Derived object;
  Right *base = &object;
  const std::type_info &dynamic = typeid(*base);
  if (dynamic != typeid(Derived) || dynamic == typeid(Right)) return 1;
  if (typeid(const int&) != typeid(int) || typeid(const int*) == typeid(int*)) return 2;
  if (typeid(int[2]) == typeid(int[3]) || typeid(int(*)[2]) == typeid(int(*)[3])) return 3;
  if (typeid(const int[2]) != typeid(int[2])) return 4;
  if (typeid(bump()) != typeid(int) || effects) return 5;
  typeid((++effects, *(Plain*)0));
  if (effects) return 6;
  typeid((++effects, *base));
  if (effects != 1) return 7;
  try { typeid((++effects, *(Right*)0)); return 8; }
  catch (const std::bad_typeid& error) { if (!strlen(error.what())) return 9; }
  if (effects != 2) return 10;
  if (typeid(int).hash_code() != typeid(const int).hash_code()) return 11;
  if (typeid(int).before(typeid(int))) return 12;
  return typeid(int).name() != typeid(const int&).name();
}
