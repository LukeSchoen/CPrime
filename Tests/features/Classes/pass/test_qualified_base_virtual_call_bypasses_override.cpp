class QualifiedCallBase
{
protected:
  virtual int Read() const
  {
    return 40;
  }
};

class QualifiedCallDerived : public QualifiedCallBase
{
public:
  int Read() const override
  {
    return QualifiedCallBase::Read() + 2;
  }
};

int main()
{
  QualifiedCallDerived object;
  return object.Read() == 42 ? 0 : 1;
}
