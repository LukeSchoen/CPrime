struct B;

struct A
{
  virtual B& operator=(const B&);
};

struct B : A
{
  B(int value) : value(value) {}
  int value;
  /* Implicit copy assignment overrides A::operator=(const B&). */
};

B& A::operator=(const B& other)
{
  return static_cast<B&>(*this);
}

int main()
{
  B destination(1);
  B source(7);
  A& base = destination;

  base = source;
  return destination.value == 7 ? 0 : 1;
}
