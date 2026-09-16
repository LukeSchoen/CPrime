class InheritedMemberBase
{
public:
  int Value() const;
};

int InheritedMemberBase::Value() const
{
  return 4;
}

class InheritedMemberDerived : public InheritedMemberBase
{
};

int main()
{
  InheritedMemberDerived value;
  return value.Value() != 4;
}
