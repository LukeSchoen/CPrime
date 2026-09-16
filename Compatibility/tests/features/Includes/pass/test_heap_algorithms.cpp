#include <algorithm>
#include <vector>
struct Minimum {bool operator()(int a,int b)const{return a>b;}};
struct Item {int n;bool operator<(const Item& b){return n<b.n;}};
int main(){
 Item items[]={{3},{8},{1}};std::make_heap(items,items+3);if(items[0].n!=8)return 4;
 std::reverse(items,items+3);if(items[2].n!=8)return 5;
 int values[]={3,1,7,2,9,5};
 std::make_heap(values,values+6);
 if(!std::is_heap(values,values+6) || values[0]!=9)return 1;
 std::sort_heap(values,values+6);
 for(int i=1;i<6;++i)if(values[i-1]>values[i])return 2;
 std::vector<int> heap;
 for(int i=0;i<6;++i){heap.push_back(values[i]);std::push_heap(heap.begin(),heap.end(),Minimum{});}
 int previous=-1;
 while(!heap.empty()) {
  if(heap.front()<previous)return 3;
  previous=heap.front();std::pop_heap(heap.begin(),heap.end(),Minimum{});heap.pop_back();
 }
 std::make_heap(values,values); std::pop_heap(values,values);std::push_heap(values,values);
 return 0;
}
