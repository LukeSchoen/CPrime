// EXPECT_EXIT: 0
// A string-literal default argument converted to a class type keeps an array
// element node owned by a temporary scope. Instantiating a convertible
// constructor template for another argument used to recycle that node during
// deduction and corrupt the live argument type.
struct Text;
Text toStringValue(bool value);
Text toStringValue(int value);
Text toStringValue(double value);
Text toStringValue(const char *value);
Text toStringValue(const Text &value);

template <typename T> T typeInstance();

#define RequiresToString(T) typename = decltype(toStringValue(typeInstance<T>()))

struct Text
{
  Text() {}
  Text(const char *text) { (void)text; }
  template <typename T, RequiresToString(T)> explicit Text(const T &value) { (void)value; }

  Text Trim(const Text &chars = "\t\n\r ") const { (void)chars; return *this; }
};

struct Runner
{
  void Run(const Text &cmd, bool user = true)
  {
    Text fields = cmd.Trim();
    (void)fields;
    (void)user;
  }
};

int main()
{
  Runner runner;
  Text command("dir");
  Text number(2);
  runner.Run(command, 1);
  (void)number;
  return 0;
}
