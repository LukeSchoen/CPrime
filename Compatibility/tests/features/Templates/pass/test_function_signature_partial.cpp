template<class Signature> struct Signature;
template<class R,class... Args> struct Signature<R(Args...)> {
  typedef R Result;
  static int count() { return sizeof...(Args); }
};
int main() { Signature<long(int,char)>::Result x=7; return x != 7 || Signature<long(int,char)>::count()!=2; }
