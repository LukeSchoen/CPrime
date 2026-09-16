template<class T>struct Range {T data[1];T* begin(){return data;}T* end(){return data+1;}};
struct Point {int x;};
struct Loader {
 template<class T,class U>Range<T> extract(int n,U* p){Range<T> r={};return r;}
 int run(){int n=0;Point p={3};Range<Point> a=extract<Point>(0,&p);Range<Point> b=extract<Point>(0,&p);for(const unsigned& value:extract<unsigned>(0,&n)) n+=value;return n;}
};
int main(){Loader l;return l.run();}
