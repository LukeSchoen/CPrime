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
  int alpha;

  Material()
  {
    this->alpha = 1;
  }

  Material(const char *nameValue, const char *textureValue)
  {
    this->name.assign(nameValue);
    this->texture.assign(textureValue);
    this->alpha = 1;
  }

  Material(const Material &rhs)
  {
    this->name = rhs.name;
    this->texture = rhs.texture;
    this->alpha = rhs.alpha;
  }

  Material &operator=(const Material &rhs)
  {
    if (this == &rhs)
      return *this;
    this->name = rhs.name;
    this->texture = rhs.texture;
    this->alpha = rhs.alpha;
    return *this;
  }
};

template<typename T>
class HeapList
{
public:
  T *items;
  int count;
  int capacity;

  HeapList()
  {
    this->items = 0;
    this->count = 0;
    this->capacity = 0;
  }

  ~HeapList()
  {
    this->clearStorage();
  }

  void reserve(int nextCapacity)
  {
    T *next;
    int i;
    if (nextCapacity <= this->capacity)
      return;
    next = (T *)malloc(sizeof(T) * (size_t)nextCapacity);
    memset(next, 0, sizeof(T) * (size_t)nextCapacity);
    for (i = 0; i < this->count; i = i + 1)
    {
      next[i] = this->items[i];
      this->items[i].~T();
    }
    if (this->items)
      free(this->items);
    this->items = next;
    this->capacity = nextCapacity;
  }

  void push(T value)
  {
    if (this->count == this->capacity)
      this->reserve(this->capacity ? this->capacity * 2 : 2);
    this->items[this->count] = value;
    this->count = this->count + 1;
  }

  void clearStorage()
  {
    int i;
    for (i = 0; i < this->count; i = i + 1)
      this->items[i].~T();
    if (this->items)
      free(this->items);
    this->items = 0;
    this->count = 0;
    this->capacity = 0;
  }
};

static Material makeMaterial(const char *nameValue, const char *textureValue)
{
  Material material(nameValue, textureValue);
  return material;
}

int main(void)
{
  HeapList<Material> materials;
  int i;

  for (i = 0; i < 40; i = i + 1)
    materials.push(makeMaterial("mat", "texture.png"));

  if (materials.count != 40)
    return 1;
  if (!materials.items[0].name.equals("mat"))
    return 2;
  if (!materials.items[0].texture.equals("texture.png"))
    return 3;
  if (!materials.items[39].name.equals("mat"))
    return 4;
  if (!materials.items[39].texture.equals("texture.png"))
    return 5;
  return 0;
}
