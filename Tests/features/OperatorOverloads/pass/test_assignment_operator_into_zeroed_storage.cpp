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

  ~OwnedText()
  {
    if (this->text)
      free(this->text);
    this->text = 0;
  }

  OwnedText &operator=(const OwnedText &rhs)
  {
    char *next = 0;
    if (rhs.text)
    {
      size_t len = strlen(rhs.text);
      next = (char *)malloc(len + 1u);
      memcpy(next, rhs.text, len + 1u);
    }
    if (this->text)
      free(this->text);
    this->text = next;
    return *this;
  }

  void assign(const char *value)
  {
    OwnedText tmp;
    tmp.text = (char *)malloc(strlen(value) + 1u);
    strcpy(tmp.text, value);
    *this = tmp;
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
  OwnedText texture;

  Material &operator=(const Material &rhs)
  {
    this->name = rhs.name;
    this->texture = rhs.texture;
    return *this;
  }
};

int main(void)
{
  Material source;
  Material *target;

  source.name.assign("mat");
  source.texture.assign("texture.png");

  target = (Material *)malloc(sizeof(Material));
  memset(target, 0, sizeof(Material));
  *target = source;

  if (!target->name.equals("mat"))
    return 1;
  if (!target->texture.equals("texture.png"))
    return 2;

  target->~Material();
  free(target);
  return 0;
}
