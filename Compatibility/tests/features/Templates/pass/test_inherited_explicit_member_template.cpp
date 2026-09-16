struct Base { template<class T> T Read(){return T(7);} };
struct Offset { virtual ~Offset(){} int offset; };
struct Derived:Offset,Base {};
int main(){Derived x;return x.Read<int>()==7?0:1;}
