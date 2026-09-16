// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int live;
int fail_at;
struct Element {
  int value;
  Element(int v) : value(v) { if (v == fail_at) throw v; ++live; }
  ~Element() { --live; }
};
struct Owner {
  const Element elements[2] = {Element(3), Element(4)};
  Owner(bool fail) { if (fail) throw 9; }
};
int main() {
  {
    Owner value(false);
    if (live != 2 || value.elements[0].value != 3 || value.elements[1].value != 4)
      return 1;
  }
  if (live) return 2;
  try { Owner value(true); return 3; } catch (int value) { if (value != 9) return 4; }
  if (live) return 5;
  fail_at = 4;
  try { Owner value(false); return 6; } catch (int value) { if (value != 4) return 7; }
  return live != 0;
}
