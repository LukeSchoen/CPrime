// EXPECT_EXIT: 0

#include <stddef.h>

template<typename T>
class Box
{
public:
  T* operator[](size_t index) { return 0; }
};

struct Entry
{
  int first;
  int second;
};

int main(void)
{
  Box<Entry> box;
  (void)box;
  return 0;
}
