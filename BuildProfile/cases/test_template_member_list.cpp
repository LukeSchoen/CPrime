// PROFILE_NAME: cpp.template.member.list

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

  int size()
  {
    return this->count;
  }
};

int main(void)
{
  List<int> numbers;
  int i;
  for (i = 0; i < 1000; i = i + 1)
    numbers.push(i);
  return numbers.size() == 1000 ? 0 : 1;
}
