// EXPECT_EXIT: 0

typedef void LogCallback(const char *, ...);

class Reporter
{
public:
  static int Set(LogCallback *callback);
  static int Set(int value);
};

int Reporter::Set(LogCallback *callback)
{
  return callback ? 8 : 7;
}

int Reporter::Set(int value)
{
  return value;
}

int main()
{
  return Reporter::Set((LogCallback *)0) == 7 ? 0 : 1;
}
