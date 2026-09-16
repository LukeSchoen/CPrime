#include <unordered_set>
#include <string>
int main(){
  std::unordered_set<std::string> values;
  if(!values.insert("one").second || values.insert("one").second)return 1;
  values.insert("two");values.insert("three");
  auto stable=values.find("one");const std::string* address=&*stable;
  values.reserve(200);if(address!=&*values.find("one"))return 2;
  auto copy=values;copy.erase("two");if(copy.size()!=2 || values.size()!=3)return 3;
  auto next=copy.erase(copy.begin());if(copy.size()!=1)return 4;
  int n=0;for(const auto& key:values){if(key.empty())return 5;++n;}
  values.clear();return n!=3 || !values.empty();
}
