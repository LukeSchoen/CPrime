typedef unsigned int Packed;
struct Vector { float x; };
struct Color {
  Vector value;
  inline operator Packed() const {return 7;}
  inline operator Vector() const {return value;}
};
unsigned int use(unsigned int a,unsigned int b){return a+b;}
int main(){Color c;return use(c,c)!=14;}
