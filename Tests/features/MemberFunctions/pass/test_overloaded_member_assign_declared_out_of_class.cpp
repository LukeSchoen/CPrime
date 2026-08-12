// EXPECT_EXIT: 0
class cpcString
{
  int selected;

  void Assign(const char *text);
  void Assign(cpcString *rhs);
};

void cpcString::Assign(const char *text)
{
  selected = 1;
}

void cpcString::Assign(cpcString *rhs)
{
  selected = 2;
}

int main(void)
{
  cpcString value;
  cpcString other;

  value.selected = 0;
  other.selected = 0;

  value.Assign("hello");
  if (value.selected != 1)
    return 1;

  value.Assign(&other);
  if (value.selected != 2)
    return 2;

  return 0;
}
