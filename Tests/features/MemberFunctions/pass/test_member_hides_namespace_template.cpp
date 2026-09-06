#include <utility>
namespace std { template<class T> struct Holder { int value; Holder(int n):value(n){} Holder& operator=(Holder other){swap(other);return *this;} void swap(Holder& other){int n=value;value=other.value;other.value=n;} }; }
int main(){std::Holder<int> a(1),b(2);a=b;return a.value==2?0:1;}
