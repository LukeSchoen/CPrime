// EXPECT_EXIT: 0

#include <stdlib.h>
#include <string.h>

class Text
{
public:
  char *data;
  Text() { this->data = 0; }
  ~Text() { if (this->data) free(this->data); this->data = 0; }
  Text &operator=(const Text &rhs)
  {
    char *next = 0;
    if (rhs.data)
    {
      size_t len = strlen(rhs.data);
      next = (char *)malloc(len + 1u);
      memcpy(next, rhs.data, len + 1u);
    }
    if (this->data)
      free(this->data);
    this->data = next;
    return *this;
  }
  void assign(const char *value)
  {
    Text tmp;
    tmp.data = (char *)malloc(strlen(value) + 1u);
    strcpy(tmp.data, value);
    *this = tmp;
  }
  int equals(const char *value) { return this->data && strcmp(this->data, value) == 0; }
};

template<typename T>
class List
{
public:
  T *items;
  long long size;
  long long capacity;
  List() { this->items = 0; this->size = 0; this->capacity = 0; }
  ~List() { if (this->items) free(this->items); this->items = 0; this->size = 0; this->capacity = 0; }
  List<T> &operator=(const List<T> &rhs)
  {
    if (this == &rhs)
      return *this;
    if (this->items)
      free(this->items);
    this->items = 0;
    this->size = rhs.size;
    this->capacity = rhs.size;
    if (rhs.size > 0)
    {
      this->items = (T *)malloc(sizeof(T) * (size_t)rhs.size);
      memcpy(this->items, rhs.items, sizeof(T) * (size_t)rhs.size);
    }
    return *this;
  }
  void resize(long long nextSize)
  {
    if (this->items)
      free(this->items);
    this->items = (T *)malloc(sizeof(T) * (size_t)nextSize);
    memset(this->items, 0, sizeof(T) * (size_t)nextSize);
    this->size = nextSize;
    this->capacity = nextSize;
  }
};

class Resource
{
public:
  long long type;
  Text name;
  List<int> buffer;
  long long count;
};

int main(void)
{
  Resource source;
  Resource *items;
  memset(&source, 0, sizeof(source));
  source.type = 2;
  source.name.assign("color0");
  source.buffer.resize(64);
  source.count = 123;

  items = (Resource *)malloc(sizeof(Resource) * 2u);
  memset(items, 0, sizeof(Resource) * 2u);
  items[0] = source;

  if (items[0].type != 2)
    return 1;
  if (!items[0].name.equals("color0"))
    return 2;
  if (items[0].buffer.size != 64)
    return 3;
  if (items[0].count != 123)
    return 4;
  items[0].~Resource();
  free(items);
  return 0;
}
