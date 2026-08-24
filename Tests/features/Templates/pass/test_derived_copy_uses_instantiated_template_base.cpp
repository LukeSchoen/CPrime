template<typename T>
class Array
{
public:
  T values[2];

  Array() { }

  Array(const Array &other)
  {
    values[0] = other.values[0];
    values[1] = other.values[1];
  }
};

class Derived : public Array<int>
{
public:
  Derived Clone() const
  {
    Derived result = *this;
    return result;
  }
};

int main()
{
  Derived value;
  value.values[0] = 13;
  value.values[1] = 29;
  Derived copy = value.Clone();
  return copy.values[0] == 13 && copy.values[1] == 29 ? 0 : 1;
}
