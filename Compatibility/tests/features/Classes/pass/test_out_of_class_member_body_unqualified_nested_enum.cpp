class Owner
{
public:
  enum Mode
  {
    Mode_None,
    Mode_Other,
  };

  int value(Mode mode = Mode_None);
};

int Owner::value(Mode mode)
{
  if (mode != Mode_None)
    return 1;
  return 0;
}

int main()
{
  Owner owner;
  if (owner.value(Owner::Mode_None) != 0)
    return 1;
  return owner.value();
}
