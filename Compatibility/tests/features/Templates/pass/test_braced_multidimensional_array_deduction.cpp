#include <stddef.h>
template<size_t R,size_t C> int area(int base,const unsigned char(&rows)[R][C]){int sum=base;for(size_t y=0;y<R;++y)for(size_t x=0;x<C;++x)sum+=rows[y][x];return sum+(int)(100*R+10*C);}
template<class T,size_t N> int length(const T(&items)[N]){return sizeof(T)*N;}
int main(){return area(7,{{1,2,3},{4,5,6}})!=258 || length({1,2,3})!=12;}
