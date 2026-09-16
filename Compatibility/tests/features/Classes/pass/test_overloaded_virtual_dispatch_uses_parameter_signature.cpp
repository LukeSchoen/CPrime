struct FirstVisit
{
};

struct SecondVisit
{
};

class OverloadedVisitor
{
public:
  virtual int Visit(const FirstVisit&)
  {
    return 1;
  }

  virtual int Visit(const SecondVisit&)
  {
    return 2;
  }
};

class ConcreteVisitor : public OverloadedVisitor
{
public:
  int Visit(const FirstVisit&) override
  {
    return 10;
  }

  int Visit(const SecondVisit&) override
  {
    return 20;
  }
};

int main()
{
  ConcreteVisitor visitor;
  OverloadedVisitor* base = &visitor;
  FirstVisit first;
  SecondVisit second;
  return base->Visit(first) == 10 && base->Visit(second) == 20 ? 0 : 1;
}
