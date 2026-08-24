#include <new>

template<typename T>
struct Buffer
{
  T *data;
  Buffer() : data(0) {}
};

struct Resource
{
  int kind;
  Buffer<unsigned char> buffer;

  Resource(int value) : kind(value) {}
};

int main()
{
  unsigned char storage[sizeof(Resource)];
  for (unsigned long long i = 0; i < sizeof(Resource); ++i)
    storage[i] = 0xab;
  Resource *resource = new(storage) Resource(7);
  return resource->kind == 7 && resource->buffer.data == 0 ? 0 : 1;
}

// EXPECT_EXIT: 0
