// EXPECT_EXIT: 0
#include <new>
int calls;
void* operator new(std::size_t) { ++calls; throw 7; }
int main() {
  void* scalar = ::operator new(8, std::nothrow);
  void* array = ::operator new[](16, std::nothrow);
  return scalar || array || calls != 2;
}
