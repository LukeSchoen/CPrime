#include <cstddef>
template<class T> T identity(T value){return value;}
int choose(std::nullptr_t){return 1;} int choose(int*){return 2;} int choose(int){return 3;}
std::nullptr_t global=nullptr;
std::nullptr_t value(){return global;}
int main(){auto n=identity(nullptr);int*p=n; void(*f)()=value(); std::nullptr_t a[2]={nullptr,nullptr};
 return p!=nullptr || f!=nullptr || choose(n)!=1 || choose(0)!=3 || sizeof(n)!=sizeof(void*) || sizeof(a)!=2*sizeof(void*);}
