// A definition whose qualifier names a class inside a namespace must bind the
// out-of-class body to that class: the implicit object parameter, unqualified
// member access, the constructor init list, the destructor, and cv-qualifiers.

namespace ns
{
  struct Widget
  {
    int value;

    Widget();
    Widget(int initial);
    ~Widget();
    int bump(int by);
    int read() const;
  };
}

ns::Widget::Widget()
  : value(0)
{
}

ns::Widget::Widget(int initial)
  : value(initial)
{
}

ns::Widget::~Widget()
{
}

int ns::Widget::bump(int by)
{
  value += by;
  return value;
}

int ns::Widget::read() const
{
  return value;
}

int main()
{
  ns::Widget widget(3);
  if (widget.read() != 3)
    return 1;
  if (widget.bump(4) != 7 || widget.read() != 7)
    return 2;
  ns::Widget zero;
  return zero.read() == 0 ? 0 : 3;
}
