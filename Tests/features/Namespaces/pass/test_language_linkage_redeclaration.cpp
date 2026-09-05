// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
extern "C" int inherited(int);
int inherited(int value) { return value + 1; }
extern "C" {
int before(int value) { return value + 2; }
extern "C++" {
int overloaded(int value) { return value + 3; }
int overloaded(double value) { return (int)value + 4; }
}
int after(int value) { return value + 5; }
}
int restored(int value) { return value + 6; }
int restored(double value) { return (int)value + 7; }
int mixed(double value) { return (int)value + 8; }
extern "C" int mixed(int value) { return value + 9; }
int main() {
  return inherited(1) != 2 || before(2) != 4 || after(3) != 8
      || overloaded(4) != 7 || overloaded(5.0) != 9
      || restored(6) != 12 || restored(7.0) != 14
      || mixed(8.0) != 16 || mixed(9) != 18;
}
