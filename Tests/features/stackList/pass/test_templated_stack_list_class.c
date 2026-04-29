// EXPECT_EXIT: 0

template<typename T>
class List
{
  T items[8];
  int count;
  int capacity;

  List()
  {
    this->count = 0;
    this->capacity = 8;
  }

  ~List()
  {
    this->count = 0;
  }

  void push(T value)
  {
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
  numbers.push(3);
  numbers.push(4);
  if (numbers.size() != 2)
    return 1;
  if (numbers.get(0) != 3)
    return 2;
  if (numbers.get(1) != 4)
    return 3;
  return 0;
}
