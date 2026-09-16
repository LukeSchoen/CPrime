struct Extractor {
 template<class T,class U> T read(int n,U* value){return T(n+*value);}
};int main(){Extractor e;short n=7;return e.read<long>(3,&n)!=10;}
