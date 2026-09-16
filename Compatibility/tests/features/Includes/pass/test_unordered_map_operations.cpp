#include <unordered_map>
#include <string>
struct Collision { size_t operator()(int)const{return 1;} };
int main(){
  std::unordered_map<int,std::string,Collision> values;
  for(int i=0;i<80;++i)values[i]="value";
  std::string* stable=&values[17];
  values.reserve(400);
  if(stable!=&values[17] || values.size()!=80)return 1;
  if(values.insert(std::make_pair(17,std::string("duplicate"))).second)return 2;
  for(int i=0;i<80;i+=2)if(values.erase(i)!=1)return 3;
  if(values.count(17)!=1 || values.count(18)!=0)return 4;
  std::unordered_map<int,std::string,Collision> copy=values;
  copy[17]="changed";if(values[17]!="value")return 5;
  int count=0;for(const auto& value:copy){if(value.first%2!=1)return 6;++count;}
  if(count!=40)return 7;
  auto moved=std::move(copy);moved.clear();return !moved.empty();
}
