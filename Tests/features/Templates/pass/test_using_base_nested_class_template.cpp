struct Base
{
  template<class T> struct Box {};
};

struct Derived : Base
{
  using Base::Box;
  int check()
  {
    Box<int> box;
    (void)box;
    return 0;
  }
};

int main()
{
  Derived d;
  return d.check();
}
