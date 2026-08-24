struct Reporter
{
  static int callback;
  static void Set(int value);
  static int Get();
};

int Reporter::callback = 0;

void Reporter::Set(int value)
{
  callback = value;
}

int Reporter::Get()
{
  return callback;
}

int main()
{
  Reporter::Set(29);
  return Reporter::Get() == 29 ? 0 : 1;
}
