// EXPECT_EXIT: 0
#include <stdlib.h>
#include <string.h>

class OwnedText
{
public:
  OwnedText();
  OwnedText(const char *text);
  OwnedText(const OwnedText other);
  ~OwnedText();

public:
  char *data;
  int length;
};

static char *copy_text(const char *text, int length)
{
  char *out = (char *)malloc((size_t)length + 1u);
  memcpy(out, text, (size_t)length);
  out[length] = 0;
  return out;
}

OwnedText::OwnedText()
{
  this->data = copy_text("", 0);
  this->length = 0;
}

OwnedText::OwnedText(const char *text)
{
  this->length = (int)strlen(text);
  this->data = copy_text(text, this->length);
}

OwnedText::OwnedText(const OwnedText other)
{
  this->length = other.length;
  this->data = copy_text(other.data, other.length);
}

OwnedText::~OwnedText()
{
  if (this->data)
  {
    this->data[0] = '!';
    free(this->data);
    this->data = 0;
  }
  this->length = -1;
}

static OwnedText make_text(void)
{
  OwnedText out("copy me");
  return out;
}

int main(void)
{
  OwnedText returned = make_text();
  if (returned.length != 7)
    return 1;
  if (strcmp(returned.data, "copy me") != 0)
    return 2;
  return 0;
}
