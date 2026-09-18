// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_shared_mutex. <shared_mutex> is missing from the runtime.
#include <shared_mutex>

int main() {
  std::shared_mutex mutex;
  {
    std::shared_lock<std::shared_mutex> shared(mutex);
    (void)shared;
  }
  {
    std::lock_guard<std::shared_mutex> exclusive(mutex);
    (void)exclusive;
  }
  return 0;
}
