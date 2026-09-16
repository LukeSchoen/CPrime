class QualifiedCase
{
public:
  enum Kind
  {
    None,
    First,
  };

  int Select(Kind kind);
};

int QualifiedCase::Select(Kind kind)
{
  switch (kind)
  {
  case QualifiedCase::None:
    return 0;
  case QualifiedCase::First:
    return 1;
  }
  return -1;
}

int main()
{
  QualifiedCase value;
  return value.Select(QualifiedCase::First) == 1 ? 0 : 1;
}
