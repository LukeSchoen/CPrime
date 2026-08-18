struct InheritedConstRefVec
{
  int value;
};

class InheritedConstRefBase
{
public:
  const InheritedConstRefVec &Get() const;
  void Use(const InheritedConstRefVec &value);
};

const InheritedConstRefVec &InheritedConstRefBase::Get() const
{
  static InheritedConstRefVec value = { 1 };
  return value;
}

void InheritedConstRefBase::Use(const InheritedConstRefVec &value)
{
  (void)value;
}

class InheritedConstRefDerived : public InheritedConstRefBase
{
};

int main()
{
  InheritedConstRefDerived value;
  value.Use(value.Get());
  return 0;
}
