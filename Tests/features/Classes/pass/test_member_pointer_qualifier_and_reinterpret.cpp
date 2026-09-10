struct A { int i; };
struct B : public A {};
struct X { void f() {} int value; };
struct Y {};

int main() {
  int B::*bp = &B::i;
  const int A::*ap = static_cast<const int A::*>(bp);
  if (ap != bp) return 1;
  const int B::*cp = bp;
  if (ap != cp || cp != bp) return 2;

  void (X::*xf)() = &X::f;
  void (Y::*yf)() = reinterpret_cast<void (Y::*)()>(xf);
  if (yf != reinterpret_cast<void (Y::*)()>(xf)) return 3;
  X x;
  (x.*xf)();

  int X::*xp = &X::value;
  int Y::*yp = reinterpret_cast<int Y::*>(xp);
  x.*xp = 9;
  if (x.value != 9) return 4;
  if (yp == reinterpret_cast<int Y::*>(static_cast<int X::*>(0))) return 5;
  return 0;
}
