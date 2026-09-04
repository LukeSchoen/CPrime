template<int Size>
struct SharedPool
{
};

struct FirstItem
{
  int Kind() const
  {
    return 1;
  }
};

struct SecondItem
{
  int Kind() const
  {
    return 2;
  }
};

class ItemFactory
{
public:
  int Run();

private:
  template<class ItemType, int Size>
  int Create(SharedPool<Size>& pool);
};

template<class ItemType, int Size>
int ItemFactory::Create(SharedPool<Size>&)
{
  ItemType item;
  return item.Kind();
}

int ItemFactory::Run()
{
  SharedPool<4> pool;
  return Create<FirstItem>(pool) * 10 + Create<SecondItem>(pool);
}

int main()
{
  ItemFactory factory;
  return factory.Run() == 12 ? 0 : 1;
}
