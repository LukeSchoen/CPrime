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
class InnerList
{
public:
  T *items;
  long long size;
  InnerList() { this->items = 0; this->size = 0; }
  ~InnerList() { if (this->items) free(this->items); this->items = 0; this->size = 0; }
  InnerList<T> &operator=(const InnerList<T> &rhs)
  {
    if (this == &rhs)
      return *this;
    if (this->items)
      free(this->items);
    this->items = 0;
    this->size = rhs.size;
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
  }
};

class Resource
{
public:
  long long type;
  Text name;
  InnerList<int> buffer;
  long long count;
};

template<typename T>
class Holder
{
public:
  T *items;
  long long size;

  Holder() { this->items = 0; this->size = 0; }
  ~Holder()
  {
    for (long long i = 0; i < this->size; i = i + 1)
      this->items[i].~T();
    if (this->items)
      free(this->items);
  }

  void copyToNewStorage(void)
  {
    T *next = (T *)malloc(sizeof(T) * 2u);
    long long i;
    memset(next, 0, sizeof(T) * 2u);
    for (i = 0; i < this->size; i = i + 1)
    {
      next[i] = this->items[i];
      this->items[i].~T();
    }
    free(this->items);
    this->items = next;
  }
};

int main(void)
{
  Holder<Resource> holder;
  holder.items = (Resource *)malloc(sizeof(Resource));
  memset(holder.items, 0, sizeof(Resource));
  holder.size = 1;
  holder.items[0].type = 2;
  holder.items[0].name.assign("color0");
  holder.items[0].buffer.resize(64);
  holder.items[0].count = 123;

  holder.copyToNewStorage();

  if (holder.items[0].type != 2)
    return 1;
  if (!holder.items[0].name.equals("color0"))
    return 2;
  if (holder.items[0].buffer.size != 64)
    return 3;
  if (holder.items[0].count != 123)
    return 4;
  return 0;
}
