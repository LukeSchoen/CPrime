namespace compatibility
{
class Reporter
{
public:
  int First(int value, ...);
};

int Reporter::First(int value, ...)
{
  return value;
}
}

int main()
{
  compatibility::Reporter reporter;
  return reporter.First(7, 8, 9) == 7 ? 0 : 1;
}
