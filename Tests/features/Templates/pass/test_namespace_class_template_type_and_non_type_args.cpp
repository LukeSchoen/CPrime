#include <stddef.h>

namespace compatibility
{
template<class T, size_t InitialSize>
class FixedArray
{
public:
  FixedArray() : used(0) {}

  void Push(T value)
  {
    data[used++] = value;
  }

  T data[InitialSize];
  size_t used;
};

struct Owner
{
  FixedArray<int*, sizeof(int)> values;
};
}

int main()
{
  compatibility::Owner owner;
  int value = 23;
  owner.values.Push(&value);
  return owner.values.used == 1 && *owner.values.data[0] == 23 ? 0 : 1;
}
