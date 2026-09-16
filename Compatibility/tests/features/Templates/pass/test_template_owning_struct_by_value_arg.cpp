// EXPECT_EXIT: 0

#include <stdlib.h>
#include <string.h>

class OwnedText
{
public:
  char *text;

  OwnedText()
  {
    this->text = 0;
  }

  OwnedText(const char *value)
  {
    this->text = 0;
    this->assign(value);
  }

  OwnedText(const OwnedText &rhs)
  {
    this->text = 0;
    this->assign(rhs.text);
  }

  ~OwnedText()
  {
    if (this->text)
      free(this->text);
    this->text = 0;
  }

  OwnedText &operator=(const OwnedText &rhs)
  {
    if (this != &rhs)
      this->assign(rhs.text);
    return *this;
  }

  void assign(const char *value)
  {
    char *next = 0;
    if (value)
    {
      size_t len = strlen(value);
      next = (char *)malloc(len + 1u);
      memcpy(next, value, len + 1u);
    }
    if (this->text)
      free(this->text);
    this->text = next;
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
  int alpha;

  Material()
  {
    this->alpha = 1;
  }

  Material(const char *nameValue, const char *textureValue)
  {
    this->name.assign(nameValue);
    this->texture.assign(textureValue);
    this->alpha = 7;
  }

  Material(const Material &rhs)
  {
    this->name = rhs.name;
    this->texture = rhs.texture;
    this->alpha = rhs.alpha;
  }
};

static Material makeMaterial(const char *nameValue, const char *textureValue)
{
  Material material(nameValue, textureValue);
  return material;
}

template<typename T>
class ValueAcceptor
{
public:
  int acceptValue(T value)
  {
    if (value.alpha != 7)
      return 3;
    if (!value.name.equals("mat"))
      return 1;
    if (!value.texture.equals("texture.png"))
      return 2;
    return 0;
  }
};

int main(void)
{
  ValueAcceptor<Material> acceptor;
  Material material("mat", "texture.png");
  Material directCopy(material);
  if (material.alpha != 7)
    return 10;
  if (directCopy.alpha != 7)
    return 11;
  if (acceptor.acceptValue(material))
    return 12;
  Material returned = makeMaterial("mat", "texture.png");
  if (returned.alpha != 7)
    return 13;
  if (!returned.name.equals("mat"))
    return 14;
  if (!returned.texture.equals("texture.png"))
    return 15;
  return acceptor.acceptValue(returned);
}
