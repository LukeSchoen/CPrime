// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: std_regex_match_groups. std::cmatch works; std::smatch must
// overload std::regex_search as well.
#include <regex>
#include <string>

int main() {
  std::regex pattern("a(b)c");
  std::string text = "abc";
  std::smatch matches;
  if (!std::regex_search(text, matches, pattern)) return 1;
  if (matches.size() != 2) return 2;
  return matches[1].str() == "b" ? 0 : 3;
}
