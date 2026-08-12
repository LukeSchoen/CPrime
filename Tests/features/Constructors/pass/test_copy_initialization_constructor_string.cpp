// EXPECT_EXIT: 0
#include <string.h>

class Text
{
public:
  const char *data;
  int length;

  Text(const char *value);
};

Text::Text(const char *value)
{
  this->data = value;
  this->length = (int)strlen(value);
}

int main(void)
{
  Text text = "hello there";
  if (text.length != 11)
    return 1;
  return strcmp(text.data, "hello there") == 0 ? 0 : 2;
}
