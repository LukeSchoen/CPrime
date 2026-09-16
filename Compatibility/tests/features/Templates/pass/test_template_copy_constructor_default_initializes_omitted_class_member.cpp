// EXPECT_EXIT: 0

template<typename T>
struct TemplateArrowCache
{
  TemplateArrowCache() : marker(0) {}

  int marker;
};

template<typename T>
struct TemplateIterator
{
  TemplateIterator(T input) : value(input) {}

  TemplateIterator(const TemplateIterator &other)
    : value(other.value)
  {
  }

  T value;
  TemplateArrowCache<T> cache;
};

int main()
{
  TemplateIterator<int> source(41);
  long long poisonedStorage[2];
  poisonedStorage[0] = -1;
  poisonedStorage[1] = -1;

  TemplateIterator<int> *copy =
    new ((void *)poisonedStorage) TemplateIterator<int>(source);
  if (copy->value != 41)
    return 1;
  if (copy->cache.marker != 0)
    return 2;
  return 0;
}
