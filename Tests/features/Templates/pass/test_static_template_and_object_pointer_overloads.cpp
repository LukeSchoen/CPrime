struct Dispatch {
 static int choose(const void *value, long long count) { return value ? (int)count : -1; }
 static int choose(int value) { return value + 10; }
 template<class T> static int choose(const T &value) { return sizeof(T); }
 static int read(const char *text) { return choose(text, 7); }
};
int main() { const char *p="x"; if(Dispatch::choose(p,9)!=9)return 1; if(Dispatch::choose(3)!=13)return 2; return Dispatch::read(p)!=7; }
