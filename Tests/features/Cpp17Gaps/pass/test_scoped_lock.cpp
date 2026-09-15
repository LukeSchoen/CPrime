// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_scoped_lock. std::lock_guard works; std::scoped_lock does
// not exist.
#include <mutex>

int main() {
  std::mutex mutex;
  {
    std::scoped_lock lock(mutex);
    (void)lock;
  }
  return 0;
}
