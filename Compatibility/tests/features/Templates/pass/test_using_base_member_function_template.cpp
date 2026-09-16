struct Base
{
  template<class T> T value(T input) { return input + 1; }
};

struct Derived : private Base
{
  using Base::value;
};

int main()
{
  Derived d;
  return d.value(41) != 42;
}
