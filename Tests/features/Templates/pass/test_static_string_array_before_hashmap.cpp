#include <string>
const std::string type_names[] = { "bool", "byte", "double", "int", "float" };
#include <unordered_map>
int main() {
  std::unordered_map<std::string, int> map;
  map["one"] = 1;
  return map.at("one") != 1 || type_names[3] != "int";
}
