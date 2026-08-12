// EXPECT_EXIT: 0

class LargeValue
{
public:
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;

  LargeValue()
  {
    this->a = 0;
    this->b = 0;
    this->c = 0;
    this->d = 0;
    this->e = 0;
    this->f = 0;
  }
};

template<typename T>
class ValueAcceptor
{
public:
  int acceptValue(T value)
  {
    if (value.a != 1)
      return 1;
    if (value.b != 2)
      return 2;
    if (value.c != 3)
      return 3;
    if (value.d != 4)
      return 4;
    if (value.e != 5)
      return 5;
    if (value.f != 6)
      return 6;
    return 0;
  }
};

int main(void)
{
  ValueAcceptor<LargeValue> acceptor;
  LargeValue value;
  value.a = 1;
  value.b = 2;
  value.c = 3;
  value.d = 4;
  value.e = 5;
  value.f = 6;
  return acceptor.acceptValue(value);
}
