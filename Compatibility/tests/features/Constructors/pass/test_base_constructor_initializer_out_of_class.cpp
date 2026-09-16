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
  DerivedCtorInit value(37);
  return value.x == 37 ? 0 : 1;
}
