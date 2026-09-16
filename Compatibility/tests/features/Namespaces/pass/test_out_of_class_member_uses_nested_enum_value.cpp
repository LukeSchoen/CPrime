struct OutOfClassEnumOwner
{
  enum Mode
  {
    None,
    Other
  };

  int Check(Mode mode);
};

int OutOfClassEnumOwner::Check(Mode mode)
{
  return mode != None;
}

int main()
{
  OutOfClassEnumOwner owner;
  return owner.Check(OutOfClassEnumOwner::None);
}
