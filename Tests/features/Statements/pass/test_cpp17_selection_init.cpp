// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: Clang llvmorg-18.1.8 SemaCXX/cxx1z-init-statement.cpp.
int live;
struct Guard { Guard() { ++live; } ~Guard() { --live; } };
int main() {
  int count = 0;
  if (int value = 3; value == 3) count += value; else return 1;
  if (++count; count != 4) return 2;
  if (Guard guard; live != 1) return 3;
  if (live) return 4;
  switch (int value = 7; value) {
    case 7: count += value; break;
    default: return 5;
  }
  switch (++count; count) { case 12: break; default: return 6; }
  return count != 12;
}
