class BaseA
{
};

class BaseB
{
};

class Derived : public BaseA, public BaseB
{
};

int main()
{
  Derived d;
  return 0;
}
