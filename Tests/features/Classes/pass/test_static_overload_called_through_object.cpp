// EXPECT_EXIT: 0

class Converter
{
public:
  static int apply(int value) { return value + 1; }
  static int apply(const char *) { return 20; }
};

int main()
{
  Converter converter;
  return converter.apply(6) == 7 ? 0 : 1;
}
