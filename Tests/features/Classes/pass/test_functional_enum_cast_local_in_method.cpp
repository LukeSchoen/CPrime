enum ResourceType : long long
{
  ResourceA = 0,
  ResourceB = 1,
  ResourceC = 2,
};

struct Store
{
  int Read(int channel)
  {
    ResourceType type = ResourceType(channel + 2);
    return type == ResourceC ? 0 : 1;
  }
};

int main()
{
  Store store;
  return store.Read(0);
}
