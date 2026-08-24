template<typename Key, typename Value>
struct Entry
{
  Key key;
  Value value;
  Entry();
};

template<typename Key, typename Value>
Entry<Key, Value>::Entry() : key(Key()), value(Value())
{
}

int main()
{
  Entry<int, short> entry;
  return entry.key + entry.value;
}
