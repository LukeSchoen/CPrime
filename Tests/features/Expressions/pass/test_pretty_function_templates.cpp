// EXPECT_EXIT: 0
#include <string.h>

static int bad;

template<class T> void f1(T value) {
  if (strcmp(__PRETTY_FUNCTION__, "void f1(T) [with T = float]"))
    bad = 1;
  (void)value;
}

template<> void f1<int>(int) {
  if (strcmp(__PRETTY_FUNCTION__, "void f1(T) [with T = int]"))
    bad = 1;
}

int main() {
  f1(0);
  f1(0.0f);
  return bad;
}
