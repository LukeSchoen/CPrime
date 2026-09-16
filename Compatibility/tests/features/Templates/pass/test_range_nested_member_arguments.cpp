template<class T>struct Vector {int Size;T* Data;};
struct Draw {Vector<unsigned> IdxBuffer;};
template<class T>struct Range {T* first;T* last;T* begin(){return first;}T* end(){return last;}};
template<class T>Range<T> iterate(T* data,const long long& length){Range<T> r={data,data+length};return r;}
template<class T>Range<const T> iterate(const T* data,const long long& length){Range<const T> r={data,data+length};return r;}
struct Render {int run(const Draw* draw){int total=0;for(unsigned& idx:iterate(draw->IdxBuffer.Data,draw->IdxBuffer.Size))total+=idx;return total;}};
int main(){unsigned a[]={3,4};Draw draw={{2,a}};Render r;return r.run(&draw)!=7;}
