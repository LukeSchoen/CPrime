namespace compatibility
{
class Base
{
public:
  int Add(int value)
  {
    return value + 1;
  }
};

class Derived : public Base
{
public:
  int Run(int value);
};

int Derived::Run(int value)
{
  return Base::Add(value);
}
}

int main()
{
  compatibility::Derived value;
  return value.Run(6) == 7 ? 0 : 1;
}
