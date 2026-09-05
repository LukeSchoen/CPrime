struct Pair {
  int a; long long b;
  Pair(int x, long long y) : a(x), b(y) {}
};
template<class T, class... Args> T create(Args... args) { return T(args...); }
int main() {
  Pair p = create<Pair>(3, 8000000000LL);
  return p.a != 3 || p.b != 8000000000LL;
}
