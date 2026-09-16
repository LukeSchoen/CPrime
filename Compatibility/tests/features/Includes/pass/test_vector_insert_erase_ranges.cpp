#include <vector>
#include <string>
int main(){
  std::vector<std::string> v;v.push_back("a");v.push_back("d");v.reserve(12);
  std::string middle[]={"b","c"};v.insert(v.begin()+1,middle,middle+2);
  if(v.size()!=4||v[0]!="a"||v[1]!="b"||v[2]!="c"||v[3]!="d")return 1;
  auto p=v.erase(v.begin()+1,v.begin()+3);if(p!=v.begin()+1||*p!="d")return 2;
  std::string more[]={"w","x","y","z"};v.insert(v.begin()+1,more,more+4);
  if(v.size()!=6||v[1]!="w"||v[4]!="z"||v[5]!="d")return 3;
  v.insert(v.begin(),v.back());if(v.front()!="d"||v.back()!="d")return 4;
  v.erase(v.begin(),v.end());return !v.empty();
}
