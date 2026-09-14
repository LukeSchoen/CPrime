// EXPECT_COMPILE_ARGS: -std=c++17
int live;
struct Guard {
  Guard() { ++live; }
  ~Guard() { --live; }
};
int main() {
  int value = 3;
  if (; value != 3) return 1;
  if (Guard(); live) return 2;
  if (++value; int selected = value) {
    if (selected != 4) return 3;
  } else return 4;
  switch (Guard(); live) {
    case 0: break;
    default: return 5;
  }
  switch (; value) {
    case 4: break;
    default: return 6;
  }
  return live;
}
