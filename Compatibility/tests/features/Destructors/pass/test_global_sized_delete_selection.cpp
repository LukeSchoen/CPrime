// EXPECT_EXIT: 0
#include <new>
#include <cstdlib>
std::size_t scalar_size, array_size;
int sized_calls, array_calls;
void operator delete(void* pointer, std::size_t size) noexcept {
  ++sized_calls; scalar_size = size; std::free(pointer);
}
void operator delete[](void* pointer, std::size_t size) noexcept {
  ++array_calls; array_size = size; std::free(pointer);
}
struct Value { int value; ~Value() {} };
struct Trivial { int value; };
struct Base { virtual ~Base() noexcept(false) {} };
struct Prefix { virtual ~Prefix() noexcept(false) {} int prefix; };
struct Derived : Prefix, Base { int values[5]; };
struct Throwing : Prefix, Base {
  int values[9];
  ~Throwing() noexcept(false) { throw 7; }
};
int main() {
  delete new int;
  if (sized_calls != 1 || scalar_size != sizeof(int)) return 1;
  delete new Trivial;
  if (sized_calls != 2 || scalar_size != sizeof(Trivial)) return 2;
  delete[] new Value[3];
  if (array_calls != 1 || array_size != sizeof(Value) * 3 + sizeof(void*)) return 3;
  delete[] new Trivial[3];
  if (array_calls != 1) return 4;
  Base* base = new Derived;
  delete base;
  if (scalar_size != sizeof(Derived)) return 5;
  try { base = new Throwing; delete base; }
  catch (int value) { if (value != 7) return 6; }
  return sized_calls != 4 || scalar_size != sizeof(Throwing);
}
