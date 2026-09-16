// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_system_error. <system_error> is missing from the runtime.
#include <system_error>

int main() {
  std::error_code empty;
  if (empty) return 1;
  std::error_code failure = std::make_error_code(std::errc::invalid_argument);
  if (!failure) return 2;
  if (failure != std::errc::invalid_argument) return 3;
  std::error_condition no_condition;
  if (no_condition) return 4;
  std::error_condition condition = std::make_error_condition(std::errc::result_out_of_range);
  if (!condition) return 5;
  if (condition != std::errc::result_out_of_range) return 6;
  if (condition.value() != 34) return 7;
  return 0;
}
