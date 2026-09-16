struct Outer
{
  struct Inner
  {
    int value;

    Inner(int initial);
    Inner(const Inner &other);
    Inner &operator=(const Inner &other);
  };
};

Outer::Inner::Inner(int initial)
  : value(initial)
{
}

Outer::Inner::Inner(const Inner &other)
  : value(other.value)
{
}

Outer::Inner &Outer::Inner::operator=(const Inner &other)
{
  value = other.value;
  return *this;
}

int main()
{
  Outer::Inner first(17);
  Outer::Inner second(first);
  Outer::Inner third(0);
  third = second;
  return third.value == 17 ? 0 : 1;
}
