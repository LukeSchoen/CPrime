static int captured = -1;

struct Probe
{
  Probe(int value);
};

Probe::Probe(int value)
{
  captured = value;
}

int RunInTemplate()
{
  const int capacity = 5;
  struct Local : Probe
  {
    Local() : Probe(capacity) {}
  };
  Local local;
  (void)local;
  return captured;
}

int RunAtFunctionScope()
{
  constexpr int capacity = 9;
  struct Local : Probe
  {
    Local() : Probe(capacity) {}
  };
  Local local;
  (void)local;
  return captured;
}

int main()
{
  if (RunInTemplate() != 5) return 1;
  if (RunAtFunctionScope() != 9) return 2;
  return 0;
}
