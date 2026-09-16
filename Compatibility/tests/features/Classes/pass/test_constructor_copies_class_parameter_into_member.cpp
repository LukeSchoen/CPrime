// EXPECT_EXIT: 0
// EXPECT_STDOUT:

struct Owned
{
  Owned(int value)
  {
    marker = value;
  }

  Owned(const Owned &other)
  {
    marker = other.marker;
  }

  long long padding[3];
  int marker;
};

struct Text
{
  Text(int value) : storage(value) {}
  Text(const Text &) = default;

  Owned storage;
};

struct Resource
{
  Resource(Text input) : name(input) {}

  Text name;
};

int main()
{
  Text source(73);
  Resource resource(source);
  if (resource.name.storage.marker != 73)
    return 1;
  return 0;
}
