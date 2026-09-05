// EXPECT_EXIT: 0
int conversions;
int destructions;

template<class T> struct Value
{
  T value;
  Value(T input) : value(input) {}
  template<class U> explicit Value(const Value<U> other) : value(T(other.value))
  { ++conversions; }
  ~Value() { ++destructions; }
};

int read_value(Value<int> input) { return input.value; }

int main()
{
  {
    Value<int> original(17);
    Value<int> copied(original);
    if (copied.value != 17 || conversions != 0) return 1;
    if (read_value(copied) != 17 || conversions != 0) return 2;
    if (destructions != 1) return 3;
  }
  return destructions != 3;
}
