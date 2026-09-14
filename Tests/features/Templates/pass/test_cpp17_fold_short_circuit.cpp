// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: GCC gcc-14.2.0 g++.dg/cpp1z/fold2.C (logical/comma operators).
int calls;
bool visit(int value) { ++calls; return value != 0; }
template<class... T> bool all(T... x) { return (... && visit(x)); }
template<class... T> bool any(T... x) { return (visit(x) || ...); }
template<class... T> void ordered(T... x) { ((calls = calls * 10 + x), ...); }
int main() {
  if (!all() || any() || calls) return 1;
  if (all(1, 0, 1) || calls != 2) return 2;
  calls = 0;
  if (!any(0, 1, 0) || calls != 2) return 3;
  calls = 0;
  ordered();
  ordered(1, 2, 3);
  return calls != 123;
}
