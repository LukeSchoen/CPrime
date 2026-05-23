class Repro
{
public:
  int caller();
  int callee(int value);
};

int Repro::caller()
{
  return callee(7);
}

int Repro::callee(int value)
{
  return value + 1;
}

int main()
{
  Repro r;
  return r.caller() == 8 ? 0 : 1;
}
