struct First { int first; int add_first(int n) { return first + n; } };
struct Second { int second; int add_second(int n) { return second + n; } };
struct Derived : First, Second { int own; };

/* A constant initializer folds the second-base subobject adjustment. */
int Derived::*global_second = &Derived::second;

struct VBase { int x; };
struct VLeft : virtual VBase {};
struct VRight : virtual VBase {};
struct VDiamond : VLeft, VRight {};

int main() {
  int First::*pf = &Derived::first;
  int Second::*ps = &Derived::second;
  int (First::*mf)(int) = &Derived::add_first;
  int (Second::*ms)(int) = &Derived::add_second;
  Derived d;
  d.first = 1;
  d.second = 2;
  if (d.*pf != 1 || d.*ps != 2) return 1;
  d.*ps = 20;
  if (d.second != 20) return 2;
  if ((d.*mf)(4) != 5) return 3;
  if ((d.*ms)(5) != 25) return 4;
  int Derived::*df = pf;
  int Derived::*ds = ps;
  if (d.*df != 1 || d.*ds != 20) return 5;
  d.*ds = 30;
  if (d.second != 30) return 6;
  d.*global_second = 40;
  if (d.second != 40) return 7;
  int (Derived::*dm)(int) = ms;
  if ((d.*dm)(1) != 41) return 8;
  VDiamond v;
  v.x = 5;
  if (&v.x != &v.VLeft::x || &v.x != &v.VRight::x || &v.x != &v.VBase::x)
    return 9;
  return 0;
}
