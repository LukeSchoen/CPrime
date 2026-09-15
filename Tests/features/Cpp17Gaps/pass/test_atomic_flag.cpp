// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: std_atomic_flag. std::atomic_flag and ATOMIC_FLAG_INIT are
// missing.
#include <atomic>

int main() {
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  if (flag.test_and_set()) return 1;
  if (!flag.test_and_set()) return 2;
  flag.clear();
  return flag.test_and_set() ? 3 : 0;
}
