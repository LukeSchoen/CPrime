class Inner
{
public:
  Inner() : value(0) {}
  Inner(int v) : value(v) {}
  Inner(const Inner &other) : value(other.value) {}

  int value;
};

class Holder
{
public:
  Holder() : field(7) {}
  Holder(Holder &&other);

  Inner field;
};

Holder::Holder(Holder &&other) : field(other.field) {}

int main()
{
  return 0;
}
