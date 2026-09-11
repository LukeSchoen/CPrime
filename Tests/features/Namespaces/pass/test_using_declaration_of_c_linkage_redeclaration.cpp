// EXPECT_EXIT: 0
/* `using Imported::c_read;` names the same C-linkage function the plain
   spelling already denotes, so the import is not a second candidate. */
extern "C" int c_read(int value) { return value + 1; }

namespace Imported {
  extern "C" int c_read(int value);
}

using Imported::c_read;

int main() {
  return c_read(1) == 2 ? 0 : 1;
}
