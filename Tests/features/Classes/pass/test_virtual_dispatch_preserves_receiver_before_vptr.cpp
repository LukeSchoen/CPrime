class StatefulVirtualBase
{
public:
  StatefulVirtualBase() : value(40) {}

  virtual int Read() const
  {
    return value;
  }

protected:
  int value;
};

class StatefulVirtualDerived : public StatefulVirtualBase
{
public:
  StatefulVirtualDerived()
  {
    value = 42;
  }

  int Read() const override
  {
    return value;
  }
};

int main()
{
  StatefulVirtualDerived object;
  StatefulVirtualBase* base = &object;
  return base->Read() == 42 ? 0 : 1;
}
