// EXPECT_EXIT: 0
int g;

struct Widget
{
  int value;

  Widget(int seed);
  ~Widget();
};

Widget::Widget(int seed)
{
  this->value = seed;
  g += seed;
}

Widget::~Widget()
{
  g += this->value;
}

int main(void)
{
  {
    struct Widget w(7);
    if (g != 7)
      return 1;
    if (w.value != 7)
      return 2;
  }
  return g == 14 ? 0 : 3;
}
