// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: polymorphic_allocator. <memory_resource> offers
// memory_resource and a buffer resource but no std::pmr::polymorphic_allocator,
// so the pmr containers cannot be built.

#include <memory_resource>

int main()
{
  std::pmr::polymorphic_allocator<int> allocator;
  int *block = allocator.allocate(4);
  allocator.deallocate(block, 4);
  return 0;
}
