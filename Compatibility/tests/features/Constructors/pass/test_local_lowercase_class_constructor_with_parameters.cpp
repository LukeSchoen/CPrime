// EXPECT_EXIT: 0
class clString
{
public:
  int length;

  clString(const char *text);
};

clString::clString(const char *text)
{
  this->length = text[0] ? 1 : 0;
}

int main(void)
{
  clString text("x");
  return text.length == 1 ? 0 : 1;
}
