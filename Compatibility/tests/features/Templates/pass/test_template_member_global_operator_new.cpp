// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Isrc/third-party/win32-sdk/include
#include <stdlib.h>

static int allocations;

void *operator new(__SIZE_TYPE__ size) {
  ++allocations;
  return malloc(size);
}

template <class T> struct allocator_user {
  T *allocate() {
    return static_cast<T *>(operator new(sizeof(T)));
  }

  int operator()(int value) { return value + 1; }

  int call_member_operator() { return operator()(4); }
};

int main() {
  allocator_user<int> user;
  int *value = user.allocate();
  if (!value || allocations != 1)
    return 1;
  free(value);
  return user.call_member_operator() != 5;
}
