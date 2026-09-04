class Base
{
public:
  Base(int input) : value(input) {}
  int value;
};

class Derived : public Base
{
public:
  Derived(int input) : Base(input) {}
};

int main()
{
  Derived object(29);
  return object.value == 29 ? 0 : 1;
}
