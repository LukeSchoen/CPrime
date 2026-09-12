// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
enum ResourceType : long long
{
  ResourceA = 0,
  ResourceB = 1,
  ResourceC = 2,
};

int run()
{
  int channel = 0;
  ResourceType type = ResourceType(channel + 2);
  return type == ResourceC ? 0 : 1;
}
}

namespace cpc_case_1
{
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

int run()
{
  int channel = 0;
  return Use(ResourceType(channel + 2), 0);
}
}

namespace cpc_case_2
{
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

int run()
{
  Store store;
  return store.Read(0);
}
}

namespace cpc_case_3
{
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

int run()
{
  Store store;
  return store.Read(0, "name");
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  if (int code = cpc_case_3::run()) { return code; }
  return 0;
}
