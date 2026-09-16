// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: aligned_new. std::align_val_t and the aligned operator new and
// delete overloads are missing from <new>.

#include <new>

int main()
{
  void *block = ::operator new(64, std::align_val_t{32});
  ::operator delete(block, std::align_val_t{32});
  return 0;
}
