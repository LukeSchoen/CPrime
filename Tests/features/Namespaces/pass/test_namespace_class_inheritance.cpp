namespace compatibility
{
class Base
{
public:
  Base() : value(7) {}
  int value;
};

class Derived : public Base
{
public:
  Derived() : Base() {}
  int Read() const { return value; }
};
}

int main()
{
  compatibility::Derived object;
  return object.Read() == 7 ? 0 : 1;
}
