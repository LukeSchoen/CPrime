#include <stdio.h>

struct ListLike
{
  int *data;
  int size;

  ListLike()
  {
    data = 0;
    size = 0;
  }

  ~ListLike()
  {
    printf("destructor: data=%p size=%d\n", data, size);
  }
};

struct Owner
{
  ListLike list;

  Owner()
  {
    printf("constructor: data=%p size=%d\n", list.data, list.size);
  }
};

int main()
{
  Owner owner;
  return 0;
}
