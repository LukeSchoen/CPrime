// EXPECT_EXIT: 0
#include <stdlib.h>
int ordinary, placement;
void operator delete(void* pointer) noexcept { ++ordinary; free(pointer); }
void operator delete(void* pointer, int) noexcept { ++placement; free(pointer); }
int main() {
  delete new int;
  return ordinary != 1 || placement;
}
