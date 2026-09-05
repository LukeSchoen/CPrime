const int &minimum(const int &a, const int &b) { return a < b ? a : b; }
double &choose(bool flag, double &a, double &b) { return flag ? a : b; }
int main() {
  int a = 4, b = 9;
  if (&minimum(a, b) != &a || &minimum(b, a) != &a) return 1;
  (a < b ? a : b) = 6;
  if (a != 6 || b != 9) return 2;
  double x = 1.5, y = 2.5;
  choose(false, x, y) = 7.5;
  if (x != 1.5 || y != 7.5 || &choose(true, x, y) != &x) return 3;
  char c = 1, d = 2;
  char &r = true ? c : d;
  r = 5;
  return c != 5 || d != 2;
}
