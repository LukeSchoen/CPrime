#include <stddef.h>
namespace {
enum Angle { zero, quarter };
template<Angle A, size_t X, size_t Y, size_t N> struct Coordinates;
template<size_t X, size_t Y, size_t N> struct Coordinates<zero,X,Y,N> {
 static const size_t oldX = X;
 static const size_t oldY = Y;
};
template<Angle A,size_t X,size_t Y,size_t N> struct Coordinates {
 static const size_t oldX = N - 1 - Coordinates<static_cast<Angle>(A-1),X,Y,N>::oldY;
 static const size_t oldY = Coordinates<static_cast<Angle>(A-1),X,Y,N>::oldX;
};
template<size_t N, Angle A> class Image {
 int* data;
public:
 Image(int*p):data(p){}
 template<size_t X,size_t Y> int& pixel() const {
   static const size_t oldX=Coordinates<A,X,Y,N>::oldX;
   static const size_t oldY=Coordinates<A,X,Y,N>::oldY;
   return data[oldY+oldX*N];
 }
};
}
int main(){int pixels[9]={0,1,2,3,4,5,6,7,8};Image<3,quarter> image(pixels);return image.pixel<1,2>()!=1;}
