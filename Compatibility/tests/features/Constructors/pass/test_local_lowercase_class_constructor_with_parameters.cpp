// EXPECT_EXIT: 0
class textValue
{
public:
  int length;

  textValue(const char *text);
};

textValue::textValue(const char *text)
{
  this->length = text[0] ? 1 : 0;
}

int main(void)
{
  textValue text("x");
  return text.length == 1 ? 0 : 1;
}
