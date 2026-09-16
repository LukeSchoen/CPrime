class DefaultArgCtor
{
public:
  DefaultArgCtor(const char *text, int length, bool checkForNulls = true);
};

DefaultArgCtor::DefaultArgCtor(const char *text, int length, bool checkForNulls)
{
  (void)text;
  (void)length;
  (void)checkForNulls;
}

int main()
{
  DefaultArgCtor value("abc", 3);
  (void)value;
  return 0;
}
