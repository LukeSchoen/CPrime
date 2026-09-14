template<typename Key, typename Value>
class Map
{
private:
  using Implementation = Value *;
  static Implementation Create();
  Implementation implementation;

public:
  Map() : implementation(Create()) {}
  bool Empty() const { return implementation == 0; }
};

template<typename Key, typename Value>
typename Map<Key, Value>::Implementation Map<Key, Value>::Create()
{
  return 0;
}

int main()
{
  Map<int, unsigned char> values;
  return values.Empty() ? 0 : 1;
}
