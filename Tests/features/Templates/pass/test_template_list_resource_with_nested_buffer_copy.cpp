// EXPECT_EXIT: 0

#include <stdlib.h>
#include <string.h>

class Text
{
public:
  char *data;

  Text()
  {
    this->data = 0;
  }

  Text(const Text &rhs)
  {
    this->data = 0;
    this->assign(rhs.data);
  }

  ~Text()
  {
    if (this->data)
      free(this->data);
    this->data = 0;
  }

  Text &operator=(const Text &rhs)
  {
    if (this != &rhs)
      this->assign(rhs.data);
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
    if (this->data)
      free(this->data);
    this->data = next;
  }

  int equals(const char *value)
  {
    return this->data && strcmp(this->data, value) == 0;
  }
};

template<typename T>
class List
{
public:
  T *items;
  long long size;
  long long capacity;

  List()
  {
    this->items = 0;
    this->size = 0;
    this->capacity = 0;
  }

  List(const List<T> &rhs)
  {
    this->items = 0;
    this->size = 0;
    this->capacity = 0;
    this->reserve(rhs.size);
    for (long long i = 0; i < rhs.size; i = i + 1)
      this->push(rhs.items[i]);
  }

  ~List()
  {
    this->clearStorage();
  }

  List<T> &operator=(const List<T> &rhs)
  {
    if (this == &rhs)
      return *this;
    this->clear();
    this->reserve(rhs.size);
    for (long long i = 0; i < rhs.size; i = i + 1)
      this->push(rhs.items[i]);
    return *this;
  }

  void reserve(long long capacity)
  {
    if (capacity <= this->capacity)
      return;
    T *next = (T *)malloc(sizeof(T) * (size_t)capacity);
    memset(next, 0, sizeof(T) * (size_t)capacity);
    for (long long i = 0; i < this->size; i = i + 1)
    {
      next[i] = this->items[i];
      this->items[i].~T();
    }
    if (this->items)
      free(this->items);
    this->items = next;
    this->capacity = capacity;
  }

  void resize(long long nextSize)
  {
    this->reserve(nextSize);
    for (long long i = this->size; i < nextSize; i = i + 1)
      memset(this->items + i, 0, sizeof(T));
    this->size = nextSize;
  }

  void push(T value)
  {
    if (this->size == this->capacity)
      this->reserve(this->capacity ? this->capacity * 2 : 8);
    memset(this->items + this->size, 0, sizeof(T));
    this->items[this->size] = value;
    this->size = this->size + 1;
  }

  void clear()
  {
    for (long long i = 0; i < this->size; i = i + 1)
      this->items[i].~T();
    this->size = 0;
  }

  void clearStorage()
  {
    this->clear();
    if (this->items)
      free(this->items);
    this->items = 0;
    this->capacity = 0;
  }
};

class Resource
{
public:
  long long type;
  Text name;
  List<int> buffer;
  long long count;
  long long width;
};

static Resource makeResource(long long type, const char *name, long long bytes)
{
  Resource res;
  memset(&res, 0, sizeof(res));
  res.type = type;
  res.name.assign(name);
  res.buffer.resize(bytes);
  res.count = bytes / 4;
  res.width = 4;
  return res;
}

int main(void)
{
  List<Resource> resources;
  resources.push(makeResource(2, "color0", 2496));
  if (resources.items[0].type != 2)
    return 20;
  if (!resources.items[0].name.equals("color0"))
    return 21;
  resources.push(makeResource(2, "normal0", 1872));
  resources.push(makeResource(2, "position0", 1872));
  resources.push(makeResource(1, "texture0", 0));
  resources.push(makeResource(3, "tint", 0));
  resources.push(makeResource(3, "alpha", 0));
  resources.push(makeResource(3, "lightPosition", 0));
  resources.push(makeResource(3, "MVP", 0));
  if (resources.items[0].type != 2)
    return 22;
  if (!resources.items[0].name.equals("color0"))
    return 23;
  resources.push(makeResource(3, "modelMat", 0));

  if (resources.size != 9)
    return 1;
  if (resources.items[0].type != 2)
    return 2;
  if (!resources.items[0].name.equals("color0"))
    return 3;
  if (resources.items[1].type != 2)
    return 4;
  if (!resources.items[1].name.equals("normal0"))
    return 5;
  if (resources.items[2].type != 2)
    return 6;
  if (!resources.items[2].name.equals("position0"))
    return 7;
  if (resources.items[0].buffer.size != 2496)
    return 8;
  if (resources.items[7].type != 3 || !resources.items[7].name.equals("MVP"))
    return 9;
  return 0;
}
