template<class T,class U> struct Pair {};
template<class C,class A> int read(const C*,Pair<const C*,A>&){return sizeof(C)+sizeof(A);}
int main(){Pair<const char*,long long> pair;return read("x",pair)!=1+sizeof(long long);}
