// EXPECT_EXIT: 0

class Value
{
public:
  int n;
  Value(int value = 0) { n = value; }
};

class PathLike
{
public:
  Value Name(bool withExtension = true) const;
  Value Extension() const;
};

Value PathLike::Name(bool withExtension) const
{
  return Value(withExtension ? 42 : 7);
}

Value PathLike::Extension() const
{
  Value name = Name();
  return name;
}

int main()
{
  PathLike path;
  return path.Extension().n == 42 ? 0 : 1;
}
