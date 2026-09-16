template <typename T>
struct MiniPool
{
  T value;
};

template <typename Key, typename Value>
struct MiniMap
{
  MiniPool<Value> entries;

  MiniMap();
  MiniMap(const MiniMap &other);
};

template <typename Key, typename Value>
MiniMap<Key, Value>::MiniMap()
  : entries()
{
}

template <typename Key, typename Value>
MiniMap<Key, Value>::MiniMap(const MiniMap &other)
  : entries()
{
  entries.value = other.entries.value;
}

int main()
{
  MiniMap<int, int> a;
  a.entries.value = 11;
  MiniMap<int, int> b(a);
  return b.entries.value == 11 ? 0 : 1;
}
