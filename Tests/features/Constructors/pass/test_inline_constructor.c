// EXPECT_EXIT: 0
int g;

struct Widget
{
  int v;

  Widget()
  {
    this->v = 7;
    g += 1;
  }
};

int main(void)
{
  struct Widget w;
  if (g != 1)
    return 1;
  return w.v == 7 ? 0 : 2;
}
