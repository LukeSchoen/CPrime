// EXPECT_EXIT: 0

class Text
{
public:
  int size;
  Text(const char *value) { size = value[0] ? 1 : 0; }
};

int main()
{
  const Text text("x");
  return text.size == 1 ? 0 : 1;
}
