struct Resource
{
  int id;

  Resource() : id(0) {}
  Resource(const Resource &) : id(-1) {}
};

struct Resources
{
  Resource values[1];

  Resource *begin() { return values; }
  Resource *end() { return values + 1; }
};

int main()
{
  Resources resources;
  resources.values[0].id = 7;
  for (Resource &resource : resources)
    return resource.id == 7 ? 0 : 1;
  return 2;
}

// EXPECT_EXIT: 0
