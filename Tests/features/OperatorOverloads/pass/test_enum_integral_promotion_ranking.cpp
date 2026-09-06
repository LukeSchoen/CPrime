// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
enum Small { small = 7 };
enum Large : unsigned { large = 0x80000000u };
enum Wide : long long { wide = 0x100000000LL };
enum Fixed : unsigned char { fixed = 9 };
int scalar(int) { return 1; }
int scalar(unsigned) { return 2; }
int scalar(long long) { return 3; }
int scalar(double) { return 4; }
int scalar(char) { return 5; }
int narrow(unsigned char) { return 1; }
int narrow(int) { return 2; }
int narrow(float) { return 3; }
int promoted(int) { return 1; }
int promoted(double) { return 2; }
int exact(const Small&) { return 1; }
int exact(int) { return 2; }
int exact(double) { return 3; }
int reference(const int&) { return 1; }
int reference(const double&) { return 2; }
int reverse(int) { return 1; }
int reverse(unsigned char) { return 2; }
struct Receiver {
  int value;
  Receiver(int) : value(1) {}
  Receiver(unsigned char) : value(2) {}
  int select(int) { return 1; }
  int select(unsigned char) { return 2; }
};
int main() {
  if (scalar(small) != 1 || scalar(large) != 2 || scalar(wide) != 3) return 1;
  Small value = small;
  if (scalar(value) != 1 || exact(value) != 1 || exact(small) != 1) return 2;
  if (narrow(fixed) != 1 || reverse(fixed) != 2) return 3;
  if (promoted(fixed) != 1 || reference(value) != 1) return 4;
  Receiver receiver(fixed);
  if (receiver.value != 2 || receiver.select(fixed) != 2) return 5;
  return 0;
}
