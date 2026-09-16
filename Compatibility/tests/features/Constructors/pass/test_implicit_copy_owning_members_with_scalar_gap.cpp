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
    return this->text && strcmp(this->text, value) == 0;
  }
};

class Vec4
{
public:
  float x;
  float y;
  float z;
  float w;
};

class Material
{
public:
  OwnedText name;
  double alpha;
  Vec4 diffuseColor;
  Vec4 ambientColor;
  Vec4 specularColor;
  OwnedText texture;
};

static Vec4 makeVec4(float x, float y, float z, float w)
{
  Vec4 value;
  value.x = x;
  value.y = y;
  value.z = z;
  value.w = w;
  return value;
}

static Material makeMaterial(void)
{
  Material material;
  material.alpha = 1.0;
  material.diffuseColor = makeVec4(1.0f, 1.0f, 1.0f, 1.0f);
  material.ambientColor = makeVec4(0.0f, 0.0f, 0.0f, 0.0f);
  material.specularColor = makeVec4(0.0f, 0.0f, 0.0f, 0.0f);
  material.name.assign("mat");
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
  if (material.diffuseColor.x != 1.0f || material.diffuseColor.w != 1.0f)
    return 4;
  return 0;
}
