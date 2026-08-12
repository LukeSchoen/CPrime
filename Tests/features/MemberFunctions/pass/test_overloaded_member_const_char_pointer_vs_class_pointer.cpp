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
  value.selected = 0;
  value.Assign("hello");
  return value.selected == 1 ? 0 : value.selected;
}
