namespace compatibility
{
int destructions = 0;

class Resource
{
public:
  Resource();
  ~Resource();
};

Resource::Resource()
{
}

Resource::~Resource()
{
  ++destructions;
}
}

int main()
{
  {
    compatibility::Resource resource;
  }
  unsigned char storage[sizeof(compatibility::Resource)];
  compatibility::Resource* resource =
    new(storage) compatibility::Resource;
  resource->~Resource();
  return compatibility::destructions == 2 ? 0 : 1;
}
