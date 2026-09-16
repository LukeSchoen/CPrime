// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: GCC gcc-14.2.0 g++.dg/cpp1z/fold2.C, member-pointer operators.
struct Item { int value; };
template<class... T> int object_left(T... x) { return (... .* x); }
template<class... T> int object_right(T... x) { return (x .* ...); }
template<class... T> int pointer_left(T... x) { return (... ->* x); }
template<class... T> int pointer_right(T... x) { return (x ->* ...); }
int main() {
  Item item = {37};
  int Item::* member = &Item::value;
  if (object_left(item, member) != 37 || object_right(item, member) != 37) return 1;
  return pointer_left(&item, member) != 37 || pointer_right(&item, member) != 37;
}
