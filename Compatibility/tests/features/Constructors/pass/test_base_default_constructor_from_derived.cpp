class BaseDefaultCtor
{
public:
  BaseDefaultCtor() { value = 73; }
  int value;
};

class DerivedDefaultCtor : public BaseDefaultCtor
{
public:
  DerivedDefaultCtor() {}
};

int main()
{
  DerivedDefaultCtor value;
  return value.value == 73 ? 0 : 1;
}
