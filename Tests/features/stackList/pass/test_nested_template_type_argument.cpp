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
  List<int> first;
  first.push(7);
  first.push(11);
  List<List<int>> nested;
  nested.push(first);
  return 0;
}
