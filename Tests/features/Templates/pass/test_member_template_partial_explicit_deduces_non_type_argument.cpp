#include <stddef.h>

namespace compatibility
{
template<class ElementType, size_t InitialSize>
class FixedArray
{
};

class Item
{
public:
  int value;
};

class Factory
{
public:
  Factory() : item(), pool() {}

  template<class NodeType, size_t InitialSize>
  NodeType* Create(FixedArray<char, InitialSize>& source);

  Item* Build();

private:
  Item item;
  FixedArray<char, 17> pool;
};

template<class NodeType, size_t InitialSize>
NodeType* Factory::Create(FixedArray<char, InitialSize>& source)
{
  (void)source;
  item.value = InitialSize;
  return &item;
}

Item* Factory::Build()
{
  return Create<Item>(pool);
}
}

int main()
{
  compatibility::Factory factory;
  return factory.Build()->value == 17 ? 0 : 1;
}
