// EXPECT_EXIT: 0

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

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
  unsigned long long begin;
  unsigned long long fillBegin;
  unsigned long long fillEnd;
  unsigned long long clearEnd;

  checksum = 0;
  begin = GetTickCount64();
  printf("[cpcPerf] running 1 tests\n");
  printf("[cpcPerf] begin: heapList.PushBackClearOneMillionFiveTimes\n");

  for (pass = 0; pass < 5; pass = pass + 1)
  {
    fillBegin = GetTickCount64();
    for (i = 0; i < 1000000; i = i + 1)
      numbers.push(i);
    fillEnd = GetTickCount64();
    checksum = checksum + numbers.size() + pass;
    numbers.clear();
    clearEnd = GetTickCount64();
    printf("[cpcPerf] pass %d: fill %llu ms, clear %llu ms\n",
           pass + 1, fillEnd - fillBegin, clearEnd - fillEnd);
  }

  printf("[cpcPerf] checksum %d\n", checksum);
  printf("[cpcPerf] end: heapList.PushBackClearOneMillionFiveTimes total %llu ms\n",
         GetTickCount64() - begin);
  return checksum == 5000010 ? 0 : 1;
}
