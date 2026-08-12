// EXPECT_EXIT: 0

#include <stdlib.h>
#include <string.h>

class OwnedText
{
public:
  char *text;
  int length;

  OwnedText()
  {
    this->text = 0;
    this->length = 0;
  }

  OwnedText(const char *value)
  {
    this->text = 0;
    this->length = 0;
    this->assign(value);
  }

  OwnedText(const OwnedText &rhs)
  {
    this->text = 0;
    this->length = 0;
    this->assign(rhs.text);
  }

  ~OwnedText()
  {
    if (this->text)
      free(this->text);
    this->text = 0;
    this->length = 0;
  }

  OwnedText &operator=(const OwnedText &rhs)
  {
    if (this == &rhs)
      return *this;
    this->assign(rhs.text);
    return *this;
  }

  void assign(const char *value)
  {
    char *next = 0;
    int nextLength = 0;
    if (value)
    {
      nextLength = (int)strlen(value);
      next = (char *)malloc((size_t)nextLength + 1u);
      memcpy(next, value, (size_t)nextLength + 1u);
    }
    if (this->text)
      free(this->text);
    this->text = next;
    this->length = nextLength;
  }

  int equals(const char *value)
  {
    if (!this->text || !value)
      return this->text == value;
    return strcmp(this->text, value) == 0;
  }
};

class Material
{
public:
  OwnedText name;
  OwnedText texture;

  Material()
  {
  }

  Material(const char *nameValue, const char *textureValue)
  {
    this->name.assign(nameValue);
    this->texture.assign(textureValue);
  }

  Material(const Material &rhs)
  {
    this->name = rhs.name;
    this->texture = rhs.texture;
  }
};

int main(void)
{
  Material original("mat", "texture.png");
  Material copy(original);

  if (!copy.name.equals("mat"))
    return 1;
  if (!copy.texture.equals("texture.png"))
    return 2;
  return 0;
}
