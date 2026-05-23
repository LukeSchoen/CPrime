// EXPECT_EXIT: 0

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
    this->items = 0;
    this->count = 0;
    this->capacity = 0;
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

  T get(int index)
  {
    return this->items[index];
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

  for (i = 0; i < 1000000; i = i + 1)
    numbers.push(i * 3);

  if (numbers.size() != 1000000)
    return 1;
  if (numbers.get(0) != 0)
    return 2;
  if (numbers.get(1) != 3)
    return 3;
  if (numbers.get(8) != 24)
    return 4;
  if (numbers.get(999999) != 2999997)
    return 5;
  return 0;
}
