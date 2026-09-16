int copies, destroyed, calls;
struct Item { int n; Item(int v=0):n(v){} Item(const Item&o):n(o.n){++copies;} ~Item(){++destroyed;} };
int live_ranges;
struct Range { Item items[2]; Range(){++live_ranges;items[0].n=3;items[1].n=5;} ~Range(){--live_ranges;} const Item* begin()const{return items;} const Item* end()const{return items+2;} };
const Range& get(const Range&r){++calls;return r;}
struct Groups { int a[2][3]; };
int main(){
 Range r;int sum=0;int before=destroyed;for(Item value:get(r)){ value.n+=1;sum+=value.n; { int value=9; if(value!=9)return 9; }} 
 if(calls!=1||sum!=10||copies!=2||destroyed!=before+2||r.items[0].n!=3)return 1;
 Groups g={{{1,2,3},{4,5,6}}};for(auto &row:g.a)for(int& v:row)v+=2;
 if(g.a[0][0]!=3||g.a[1][2]!=8)return 2;
 int total=0;for(const auto& value:r){total+=value.n;}if(total!=8)return 3;
 total=0;for(const Item& value:Range()){if(live_ranges!=2)return 4;total+=value.n;}
 if(live_ranges!=1||total!=8)return 5;
 int values[2]={2,3}; total=0;for(int value:values)for(int i=0;i<value;++i)++total;
 if(total!=5)return 6;
 total=0;for(int value:values)do{++total;}while(false);
 return total!=2;
}
