int first(int n) { return n + 3; }
int second(int n) { return n * 2; }
template<class F, F Fn> int invoke(int n) { return Fn(n); }
struct Registry {
  template<class F, F Fn> int call(int n) { return Fn(n); }
};
int main() {
  Registry r;
  if (invoke<decltype(&first), &first>(4) != 7) return 1;
  if (invoke<decltype(&second), &second>(4) != 8) return 2;
  if (r.call<decltype(&first), &first>(8) != 11) return 3;
  return r.call<decltype(&second), &second>(8) != 16;
}
