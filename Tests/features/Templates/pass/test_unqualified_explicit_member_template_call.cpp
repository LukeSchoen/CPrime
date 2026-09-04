struct Item
{
  int value;
};

class Factory
{
public:
  Factory();
  template<class T> T* Select(void* address);
  Item* Build();

private:
  Item item_;
};

Factory::Factory()
{
  item_.value = 29;
}

template<class T>
T* Factory::Select(void* address)
{
  return static_cast<T*>(address);
}

Item* Factory::Build()
{
  return Select<Item>(&item_);
}

int main()
{
  Factory factory;
  return factory.Build()->value == 29 ? 0 : 1;
}
