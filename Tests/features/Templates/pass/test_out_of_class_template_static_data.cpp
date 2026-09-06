int increment(int n){return n+3;}
typedef int(*Callback)(int);
template<class T>struct Store{static Callback call;static int count;static int unused;};
template<class T> Callback Store<T>::call=increment;
template<class T> int Store<T>::count=7;
template<class T> int Store<T>::unused=T::missing;
int main(){if(Store<int>::call(4)!=7)return 1;Store<int>::count=9;return Store<int>::count!=9||Store<char>::count!=7;}
