// EXPECT_EXIT: 0
#include <new>
#include <cstdlib>
int released;
void operator delete(void* pointer) noexcept { ++released; std::free(pointer); }
struct Value { Value() { throw 7; } };
int main() {
  try { new (std::nothrow) Value; }
  catch (int code) { if (code != 7) return 1; }
  return released != 1;
}
