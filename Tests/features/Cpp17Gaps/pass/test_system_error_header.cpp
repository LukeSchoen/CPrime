// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_system_error. <system_error> is missing from the runtime.
#include <system_error>

int main() {
  std::error_code empty;
  if (empty) return 1;
  std::error_code failure = std::make_error_code(std::errc::invalid_argument);
  if (!failure) return 2;
  return failure == std::errc::invalid_argument ? 0 : 3;
}
