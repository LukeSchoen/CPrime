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
    return this->text && strcmp(this->text, value) == 0;
  }
};

class Material
{
public:
  OwnedText name;
  double alpha;
  OwnedText texture;
};

static Material makeMaterial(void)
{
  Material material;
  material.name.assign("mat");
  material.alpha = 1.0;
  material.texture.assign("texture.png");
  return material;
}

int main(void)
{
  Material material = makeMaterial();
  if (!material.name.equals("mat"))
    return 1;
  if (!material.texture.equals("texture.png"))
    return 2;
  if (material.alpha != 1.0)
    return 3;
  return 0;
}
