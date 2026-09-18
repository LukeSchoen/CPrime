// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: string_view_search. basic_string_view has construction, size
// and indexing but no find, rfind or compare.

#include <string_view>

int main()
{
  std::string_view text("hello world");
  if (text.find("world") != 6) return 1;
  if (text.rfind('l') != 9) return 2;
  return text.compare("hello world") == 0 ? 0 : 3;
}
