struct Text {
 int value;
 template<class T> Text(const T& n):value((int)n){}
};
struct Path { Text text; Path(const Text& t):text(t){} };
struct Writer {
 int value;
 Writer(long long n=8):value((int)n){}
 Writer(const Path&,long long n=8,bool append=false):value(100+(int)n+(int)append){}
};
int main(){unsigned long long n=7;Writer writer(n);return writer.value!=7;}
