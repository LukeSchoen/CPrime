enum ResourceType : long long
{
  ResourceA = 0,
  ResourceB = 1,
  ResourceC = 2,
};

int main()
{
  int channel = 0;
  ResourceType type = ResourceType(channel + 2);
  return type == ResourceC ? 0 : 1;
}
