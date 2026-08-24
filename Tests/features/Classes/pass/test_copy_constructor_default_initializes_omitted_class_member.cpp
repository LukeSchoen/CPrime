// EXPECT_EXIT: 0

struct ArrowCache
{
  ArrowCache() : marker(0) {}

  int marker;
};

struct Iterator
{
  Iterator(int input) : value(input) {}

  Iterator(const Iterator &other)
    : value(other.value)
  {
  }

  int value;
  ArrowCache cache;
};

int main()
{
  Iterator source(41);
  long long poisonedStorage[2];
  poisonedStorage[0] = -1;
  poisonedStorage[1] = -1;

  Iterator *copy = new ((void *)poisonedStorage) Iterator(source);
  if (copy->value != 41)
    return 1;
  if (copy->cache.marker != 0)
    return 2;
  return 0;
}
