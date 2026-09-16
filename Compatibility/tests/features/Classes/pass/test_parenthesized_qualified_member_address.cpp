struct Base
{
  int value;
  int *own_address() { return &(Base::value); }
};

struct Derived : Base
{
  int *base_address() { return &(Base::value); }
  int Base::*base_pointer() { return &Base::value; }
};

int main()
{
  Derived object;
  object.value = 42;
  if (object.own_address() != &object.value)
    return 1;
  if (object.base_address() != &object.value)
    return 2;
  int Base::*member = object.base_pointer();
  if (object.*member != 42)
    return 3;
  return 0;
}
