struct First { int operator()(int n, double d) const {return n+(int)(d*10);} };
struct Second { int operator()(int n, double d) const {return 100+n+(int)(d*10);} };
struct Empty { int operator()() const {return 31;} };
template<class Tag> struct Bound {
 int value;
 template<class Function, class... Args>
 Bound(Function function,Args... args):value(function(args...)){}
 template<class Function, class... Args>
 int call(Function function,Args... args){return function(args...);}
};
int main(){
 Bound<int> a(First{},4,2.5);
 Bound<int> b(Second{},4,2.5);
 Bound<int> empty(Empty{});
 if(a.value!=29 || b.value!=129 || empty.value!=31)return 1;
 if(a.call(First{},6,3.5)!=41 || a.call(Second{},6,3.5)!=141)return 2;
 return a.call(Empty{})!=31?3:0;
}
