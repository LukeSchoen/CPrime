template<typename T>
struct Iterator
{
  T value;

  Iterator(T input) : value(input)
  {
    AdvanceToValid();
  }

  void AdvanceToValid()
  {
    value = value + T(1);
  }
};

int main()
{
  Iterator<int> iterator(8);
  return iterator.value == 9 ? 0 : 1;
}
