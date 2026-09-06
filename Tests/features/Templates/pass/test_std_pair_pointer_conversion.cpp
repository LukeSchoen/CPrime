#include <utility>
std::pair<const int*,int> find(int* ptr) { if (!ptr) return std::make_pair(nullptr,-1); return std::make_pair(ptr,2); }
int main(){int v=3; auto p=find(&v);return *p.first!=3 || p.second!=2 || find(nullptr).first!=nullptr;}
