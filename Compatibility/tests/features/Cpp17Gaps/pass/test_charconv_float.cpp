// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: charconv_float. The floating-point overloads truncate at the
// decimal point: to_chars(1.5) writes "1" and from_chars("1.5") consumes one
// character and answers 1.0.

#include <charconv>

int main()
{
  char buffer[8] = {0};
  std::to_chars_result written = std::to_chars(buffer, buffer + 8, 1.5);
  if (written.ec != std::errc()) return 1;
  if (written.ptr != buffer + 3) return 2;

  const char *text = "1.5";
  double value = 0;
  std::from_chars_result read = std::from_chars(text, text + 3, value);
  if (read.ec != std::errc()) return 3;
  if (read.ptr != text + 3) return 4;
  return value == 1.5 ? 0 : 5;
}
