// EXPECT_EXIT: 0
namespace Imported {
  int read(double) { return 3; }
  int read(char *) { return 5; }
}
namespace Combined {
  int read(int) { return 7; }
  using Imported::read;
  int read(float) { return 9; }
}
namespace Other { int read(short) { return 11; } }
namespace Reexported { using Imported::read; using Other::read; }
int main() {
  char value = 0;
  if (Combined::read(1) != 7) return 1;
  if (Combined::read(1.0) != 3) return 2;
  if (Combined::read(&value) != 5) return 3;
  if (Combined::read(1.0f) != 9) return 4;
  if (Reexported::read((short)1) != 11) return 5;
  if (Reexported::read(1.0) != 3) return 6;
  int (*selected)(short) = &Reexported::read;
  if (selected(1) != 11) return 7;
  return 0;
}
