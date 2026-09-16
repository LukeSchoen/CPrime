// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_memory_resource. <memory_resource> is missing from the
// runtime.
#include <memory_resource>

int main() {
  std::pmr::monotonic_buffer_resource pool;
  void *block = pool.allocate(16);
  if (block == nullptr) return 1;
  pool.deallocate(block, 16);
  return 0;
}
