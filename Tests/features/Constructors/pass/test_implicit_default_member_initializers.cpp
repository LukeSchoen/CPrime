// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int calls;
int next() { return ++calls; }
struct Value {
  const int tag = 7;
  int value = next();
  const int array[2] = {3, 5};
  void set(int n) { value = n; }
  int get() const { return tag == 7 && array[1] == 5 ? value : -1; }
};
struct Owner { Value member; };
Value global;
template<class T> T make(int n) { T result; result.set(n); return result; }
int main() {
  if (global.get() != 1 || calls != 1) return 1;
  Value local;
  if (local.get() != 2 || calls != 2) return 2;
  Value array[2];
  if (array[0].get() != 3 || array[1].get() != 4 || calls != 4) return 3;
  Owner owner;
  if (owner.member.get() != 5 || calls != 5) return 4;
  if (make<Value>(19).get() != 19 || calls != 6) return 5;
  return 0;
}
