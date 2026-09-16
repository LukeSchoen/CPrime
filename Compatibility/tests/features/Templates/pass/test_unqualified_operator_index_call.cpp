// EXPECT_EXIT: 0

template<typename T>
class Array
{
public:
  T values[3];
  T &operator[](int index) { return values[index]; }

  void set(int index, int value)
  {
    operator[](index) = value;
  }
};

int main()
{
  Array<int> value;
  value.set(2, 19);
  return value[2] == 19 ? 0 : 1;
}
