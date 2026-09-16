#include <map>
#include <string>
struct Compare {static int calls;bool operator()(int a,int b)const{++calls;return a<b;}};
int Compare::calls=0;
int main(){
  std::map<int,std::string,Compare> values;
  for(int i=0;i<1000;++i)values[i]="item";
  if(Compare::calls>60000)return 1;
  std::string* stable=&values[511];
  for(int i=0;i<1000;i+=2)if(values.erase(i)!=1)return 2;
  if(&values[511]!=stable || values.size()!=500)return 3;
  int expected=1;for(const auto& v:values){if(v.first!=expected)return 4;expected+=2;}
  auto last=values.end();--last;if(last->first!=999)return 5;
  if(values.lower_bound(500)->first!=501||values.upper_bound(501)->first!=503)return 6;
  auto copy=values;copy[511]="copy";if(values[511]!="item")return 7;
  for(int i=0;i<1000;++i){int key=(i*337)%1000;values.erase(key);}
  if(!values.empty())return 8;
  auto moved=std::move(copy);return moved.size()!=500;
}
