// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int live, calls;
bool should_throw;
struct Member {
  Member() { ++live; }
  ~Member() { --live; }
};
int initialize() {
  ++calls;
  if (should_throw) throw 7;
  return 9;
}
struct Implicit {
  Member member;
  const int value = initialize();
};
struct Explicit {
  Member member;
  const int value = initialize();
  Explicit() {}
};
int main() {
  { Implicit object; if (object.value != 9 || calls != 1 || live != 1) return 1; }
  { Explicit object; if (object.value != 9 || calls != 2 || live != 1) return 2; }
  if (live) return 3;
  should_throw = true;
  try { Implicit object; return 4; }
  catch (int n) { if (n != 7 || live || calls != 3) return 5; }
  try { Explicit object; return 6; }
  catch (int n) { if (n != 7 || live || calls != 4) return 7; }
  return 0;
}
