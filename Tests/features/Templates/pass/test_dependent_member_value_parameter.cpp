template<bool B,class T=void>struct Enable{};template<class T>struct Enable<true,T>{typedef T type;};
template<class T>struct Box{int value;template<class U,typename Enable<(sizeof(U)>0),int>::type=0>Box(U u):value(u){}};
int main(){Box<int>b(7);return b.value!=7;}
