// PROFILE_NAME: cpp.template.list.runtime
// EXPECT_EXIT: 0

#include <stdlib.h>

template<typename T>
class List
{
  T *items;
  int count;
  int capacity;

  List()
  {
    this->items = (T *)malloc(sizeof(T) * 8);
    this->count = 0;
    this->capacity = 8;
  }

  ~List()
  {
    free(this->items);
  }

  void push(T value)
  {
    if (this->count == this->capacity)
    {
      this->capacity = this->capacity * 2;
      this->items = (T *)realloc(this->items, sizeof(T) * this->capacity);
    }
    this->items[this->count] = value;
    this->count = this->count + 1;
  }

  void clear()
  {
    this->count = 0;
  }

  int size()
  {
    return this->count;
  }
};

int main(void)
{
  List<int> numbers;
  int pass;
  int i;
  int checksum;

  checksum = 0;
  for (pass = 0; pass < 5; pass = pass + 1)
  {
    for (i = 0; i < 200000; i = i + 1)
      numbers.push(i);
    checksum = checksum + numbers.size() + pass;
    numbers.clear();
  }

  return checksum == 1000010 ? 0 : 1;
}
