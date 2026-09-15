// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_filesystem. <filesystem> is missing from the runtime.
#include <filesystem>

int main() {
  std::filesystem::path path(".");
  if (path.empty()) return 1;
  return std::filesystem::exists(path) ? 0 : 2;
}
