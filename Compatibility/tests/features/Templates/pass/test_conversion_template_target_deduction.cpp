struct Null {
  template<class T> operator T*() const { return 0; }
  template<class C, class T> operator T C::*() const { return 0; }
};
struct Target {
  int member;
  int method() { return 4; }
};
template<class T> struct Number {
  operator T() { return 3; }
  template<class U> operator U() { return U(7); }
  template<class U> int convert() { return int(operator U()); }
};
int main() {
  Null zero;
  int *integer = zero;
  double *real = zero;
  int Target::*data = zero;
  int (Target::*method)() = zero;
  if (data != 0 || method != 0) return 1;
  Number<int> number;
  return integer != 0 || real != 0 || number.operator int() != 3
      || number.operator double() != 7 || number.convert<float>() != 7;
}
