// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: lib_string_view. <string_view> is missing from the runtime.
#include <string_view>

int main() {
  std::string_view view("ab");
  if (view.size() != 2) return 1;
  if (view[0] != 'a' || view[1] != 'b') return 2;
  return std::string_view("ab") == view ? 0 : 3;
}
