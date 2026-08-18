class OverrideBase
{
public:
  virtual int First() = 0;
  virtual bool Second() = 0;
};

class OverrideDerived : public OverrideBase
{
public:
  virtual int First() override;
  virtual bool Second() override;
};

int OverrideDerived::First()
{
  return 3;
}

bool OverrideDerived::Second()
{
  return true;
}

int main()
{
  OverrideDerived value;
  return value.First() != 3 || !value.Second();
}
