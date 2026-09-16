// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <new>
int qualified_destructions;
namespace qualified_members {
struct First {
  int value;
  First(): value(5) {}
  virtual int read() { return value; }
};
struct Second {
  int value;
  Second(): value(9) {}
  virtual int read() { return value; }
  int operator+(int v) const { return value + v; }
};
struct Derived : First, Second {
  int read() override { return 20; }
};
template<class T> struct Wrapper {
  T value;
  int read() const { return value; }
};
struct Destroyed {
  ~Destroyed() { ++qualified_destructions; }
};
}
using BaseAlias = qualified_members::Second;
int main() {
  qualified_members::Derived value;
  qualified_members::Derived* pointer = &value;
  if (value.read() != 20) return 1;
  if (value.qualified_members::First::read() != 5) return 2;
  if (pointer->qualified_members::Second::read() != 9) return 3;
  if (value.BaseAlias::value != 9) return 4;
  if (value.qualified_members::Second::operator+(3) != 12) return 5;
  qualified_members::Wrapper<int> wrapper = {7};
  if (wrapper.qualified_members::Wrapper<int>::read() != 7) return 6;
  if ((true ? wrapper.value : 0) != 7) return 7;
  if ((true ? value.BaseAlias::value : 0) != 9) return 8;
  if (!(wrapper.value < 8)) return 9;
  char storage[sizeof(qualified_members::Destroyed)];
  qualified_members::Destroyed* destroyed = new (storage) qualified_members::Destroyed;
  destroyed->qualified_members::Destroyed::~Destroyed();
  return qualified_destructions != 1;
}
