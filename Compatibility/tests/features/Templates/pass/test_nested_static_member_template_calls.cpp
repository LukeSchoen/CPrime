struct Gradient {
 template<unsigned M, unsigned N> static void blend(int &back, int front) { back = back + front * M / N; }
};
template<int Size> struct Output {
 int values[Size * Size];
 template<int R, int C> int &ref() { return values[R * Size + C]; }
};
template<class Base> struct Worker : Base {
 static const int scale = 2;
 template<unsigned M, unsigned N> static void blend(int &back, int front) { Base::template blend<M,N>(back, front); }
 template<class Result> static void set(int value, Result &out) {
   blend<1, 4>(out.template ref<1, 0>(), value);
   blend<3, 4>(out.template ref<scale - 1, 1>(), value);
 }
};
int main(){ Output<2> out = {}; Worker<Gradient>::set(8,out); return out.values[2]!=2 || out.values[3]!=6; }
