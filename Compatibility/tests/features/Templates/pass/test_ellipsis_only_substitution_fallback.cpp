struct Yes{static const bool value=true;};struct No{static const bool value=false;};
struct Explicit{explicit Explicit(int){}};
template<class T>T&& val();template<class T>void accept(T);
template<class T,class U>auto test(int)->decltype(accept<T>(val<U>()),Yes{});
template<class T,class U>No test(...);
int main(){return decltype(test<Explicit,int>(0))::value;}
