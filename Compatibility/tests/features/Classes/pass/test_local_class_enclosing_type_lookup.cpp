int adjust(int){return 99;}
namespace inner {int adjust(int n){return n+1;}
struct Store {struct Pair {int key;};int check(){enum {Offset=2};static int used=0;struct Local {static int identity(int n){return n;}static int compare(const void* p){++used;return Local::identity(adjust(((const Pair*)p)->key)+Offset);}};Pair p={7};return Local::compare(&p)+used;}};
}
int main(){inner::Store s;return s.check()!=11;}
