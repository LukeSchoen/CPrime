#include <deque>
#include <string>
int main(){
  std::deque<std::string> d;d.push_back("anchor");std::string* anchor=&d.front();
  for(int i=0;i<40;++i){d.push_front("front");d.push_back("back");}
  if(d.size()!=81||&d[40]!=anchor||*anchor!="anchor")return 1;
  auto survivor=d.begin()+40;
  for(int i=0;i<40;++i){d.pop_front();d.pop_back();}
  if(d.size()!=1||&*survivor!=anchor||d.front()!="anchor")return 2;
  std::deque<std::string> copy=d;copy.push_back("other");if(d.size()!=1||copy.size()!=2)return 3;
  const std::deque<std::string>& c=copy;int count=0;for(const auto& item:c)if(!item.empty())++count;
  if(count!=2||copy.end()-copy.begin()!=2)return 4;
  std::deque<std::string> moved(std::move(copy));if(moved.size()!=2||!copy.empty())return 5;
  moved.clear();return !moved.empty();
}
