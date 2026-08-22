enum ResourceType : long long
{
  ResourceA = 0,
  ResourceB = 1,
  ResourceC = 2,
};

int Use(ResourceType type, int value)
{
  return type == ResourceC ? value : 0;
}

int main()
{
  int channel = 0;
  return Use(ResourceType(channel + 2), 0);
}
