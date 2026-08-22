class BaseCtorInit
{
public:
  BaseCtorInit(int value = 1) { x = value; }
  int x;
};

class DerivedCtorInit : public BaseCtorInit
{
public:
  DerivedCtorInit(int value = 2);
};

DerivedCtorInit::DerivedCtorInit(int value)
  : BaseCtorInit(value)
{
}

int main()
{
  return 0;
}
