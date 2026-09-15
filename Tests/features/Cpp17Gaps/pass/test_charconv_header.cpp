// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: lib_charconv. <charconv> is missing from the runtime.
#include <charconv>

int main() {
  const char *text = "42";
  int value = 0;
  std::from_chars_result read = std::from_chars(text, text + 2, value);
  if (read.ec != std::errc()) return 1;
  if (value != 42) return 2;

  char buffer[8] = {0};
  std::to_chars_result written = std::to_chars(buffer, buffer + 8, 42);
  if (written.ec != std::errc()) return 3;
  return written.ptr == buffer + 2 ? 0 : 4;
}
