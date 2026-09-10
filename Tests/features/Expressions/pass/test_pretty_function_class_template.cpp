// EXPECT_EXIT: 0
#include <string.h>

static size_t current;
static int error;

static const char *expected[] = {
  "X<T>::X() [with T = void]",
  "X<T>::~X() [with T = void]",
  0
};

static void Verify(const char *pretty) {
  error = strcmp(pretty, expected[current++]);
}

template <typename T> struct X {
  X() { Verify(__PRETTY_FUNCTION__); }
  ~X() { Verify(__PRETTY_FUNCTION__); }
};

int main() {
  {
    X<void> value;
    if (error)
      return 1;
  }
  return error ? 2 : 0;
}
