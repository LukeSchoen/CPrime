class Base
{
public:
  Base() : value(47) {}

  int value;
};

class Derived : public Base
{
};

class Handle
{
public:
  explicit Handle(Base* base) : held(base) {}
  explicit Handle(Base& base) : held(&base) {}

  int Get() const
  {
    return held->value;
  }

private:
  Base* held;
};

int main()
{
  Derived derived;
  return Handle(derived).Get() == 47 ? 0 : 1;
}
