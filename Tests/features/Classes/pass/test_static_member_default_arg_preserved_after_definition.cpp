class StaticDefaultArg
{
public:
  static int Add(int value, int amount = 3);
};

int StaticDefaultArg::Add(int value, int amount)
{
  return value + amount;
}

int main()
{
  return StaticDefaultArg::Add(4) == 7 ? 0 : 1;
}
