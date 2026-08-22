enum ResourceType : long long
{
  ResourceA = 0,
  ResourceB = 1,
  ResourceC = 2,
};

struct Store
{
  int Get(ResourceType type, const char *name)
  {
    return type == ResourceC && name ? 0 : 1;
  }

  int Read(int channel, const char *name)
  {
    return Get(ResourceType(channel + 2), name);
  }
};

int main()
{
  Store store;
  return store.Read(0, "name");
}
