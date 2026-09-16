// Rediscovering a member interface must retain its already deduced return.
using Real=double;
template<class T> struct Number {
 T value;
 Number(T n):value(n){}
 template<class U> auto operator+(const Number<U>& other) const;
};
template<class T> template<class U> auto Number<T>::operator+(const Number<U>& other) const { return value+other.value; }
int main() {
 Number<Real> first(1.25);
 Number<double> concrete(2.5);
 auto a=first+concrete;
 Number<int> integer(4);
 auto d=integer+integer;
 Number<char> character(3);
 auto e=character+character;
 auto b=first+concrete;
 auto f=integer+integer;
 return a!=3.75 || b!=3.75 || d!=8 || e!=6 || f!=8;
}
