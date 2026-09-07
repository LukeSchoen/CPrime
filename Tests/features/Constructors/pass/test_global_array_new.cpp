// EXPECT_EXIT: 0
// GCC g++.dg/init/new3.C and new4.C exposed a crash in the constant probe.
struct Owner { static char *bytes; };
int count = 3;
char *Owner::bytes = new char[count];
int *zeros = new int[2]();
struct Item { int value; Item() : value(17) {} };
Item *items = new Item[2];
int main() {
  Owner::bytes[2] = 41;
  int result = Owner::bytes[2] != 41 || zeros[0] || zeros[1]
    || items[0].value != 17 || items[1].value != 17;
  delete[] Owner::bytes;
  delete[] zeros;
  delete[] items;
  return result;
}
