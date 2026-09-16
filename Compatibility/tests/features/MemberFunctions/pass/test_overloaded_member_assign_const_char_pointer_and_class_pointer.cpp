// EXPECT_EXIT: 0
class cpcString
{
  int selected;

  void Assign(const char *text)
  {
    selected = 1;
  }

  void Assign(cpcString *rhs)
  {
    selected = 2;
  }
};

int main(void)
{
  cpcString value;
  cpcString other;
  cpcString *ptr;

  value.selected = 0;
  other.selected = 0;
  ptr = &value;

  value.Assign("hello");
  if (value.selected != 1)
    return 1;

  value.Assign(&other);
  if (value.selected != 2)
    return 2;

  ptr->Assign("world");
  if (value.selected != 1)
    return 3;

  ptr->Assign(&other);
  if (value.selected != 2)
    return 4;

  return 0;
}
