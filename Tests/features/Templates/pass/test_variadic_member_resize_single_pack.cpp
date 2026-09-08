// EXPECT_STDOUT: 20
#include <new>
#include <stdio.h>
template<class T> struct Buffer {
  T elements[16];
  int count;
  Buffer() : count(0) {}
  template<class... Args> void Resize(int size, Args&&... args) {
    for (int i = count; i < size; ++i)
      new (&elements[i]) T(static_cast<Args&&>(args)...);
    count = size;
  }
  int Size() const { return count; }
};
int main() {
  Buffer<char> first, second;
  first.Resize(10);
  second.Resize(10, 'x');
  for (int i = 0; i < 10; ++i)
    if (first.elements[i] != 0 || second.elements[i] != 'x') return 1;
  printf("%d", first.Size() + second.Size());
  return 0;
}
