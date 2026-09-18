// EXPECT_COMPILE_ARGS: -std=c++17
// C++17 gap probe: map_node_handle. try_emplace, insert_or_assign, extract, merge
// and node_type are missing from <map> and <unordered_map>.

#include <map>
#include <string>

int main()
{
  std::map<int, std::string> source;
  source.try_emplace(1, "one");
  source.insert_or_assign(1, "uno");

  auto handle = source.extract(1);
  if (handle.key() != 1) return 1;

  std::map<int, std::string> target;
  target.insert(std::move(handle));
  return target.at(1) == "uno" ? 0 : 2;
}
