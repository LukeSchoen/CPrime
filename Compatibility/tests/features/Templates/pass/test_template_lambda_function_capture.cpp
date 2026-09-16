#include <functional>
struct Game {
 long add(long time,const std::function<long()>& f){return time+f();}
 long add(long time,const std::function<void()>& f){f();return time;}
 template<class F>long add(long time,const F& payload){using R=decltype(payload());return add(time,std::function<R()>([=]()->auto{return payload();}));}
};
int calls;void callback(){++calls;}
int main(){Game g;return g.add(2,[]{return 5L;})!=7 || g.add(4,[]{})!=4 || g.add(6,callback)!=6 || calls!=1;}
