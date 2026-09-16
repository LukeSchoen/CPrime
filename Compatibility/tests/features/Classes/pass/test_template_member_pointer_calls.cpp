// Must compile and return zero for both template ownership shapes.
template<class T> struct A {
    int value;
    int plain(int x) { return value + x; }
};
struct D {
    int value;
    template<class T> int add(T x) { return value + x; }
};
int main() {
    A<int> a;
    a.value = 7;
    int (A<int>::*p)(int) = &A<int>::plain;
    if ((a.*p)(3) != 10) return 1;
    D d;
    d.value = 11;
    int (D::*q)(int) = &D::add<int>;
    if ((d.*q)(3) != 14) return 2;
    return 0;
}
