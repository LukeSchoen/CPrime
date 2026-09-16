class GlobalConverted
{
public:
  GlobalConverted(const char *text) : value(text[0]) {}

  int value;
};

static GlobalConverted globalConverted = "A";

int main()
{
  return globalConverted.value == 'A' ? 0 : 1;
}
