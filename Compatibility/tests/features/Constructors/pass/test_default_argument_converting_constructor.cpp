struct Text{int length;Text(const char* s):length(0){while(s[length])++length;}};
int length(Text t="default"){return t.length;}
struct Session {int n;Session(Text a,Text b="second"):n(a.length+b.length){}};
int main(){Session s("first");return s.n==11 && length()==7?0:1;}
