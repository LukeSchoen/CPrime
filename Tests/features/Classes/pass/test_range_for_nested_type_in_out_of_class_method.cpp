class Core
{
public:
  struct glResource
  {
    int rType;
  };

  struct ResourceList
  {
    glResource data[1];
    int Size() const { return 1; }
    glResource& operator[](int index) { return data[index]; }
  };

  void Render();
  ResourceList m_resources;
};

void Core::Render()
{
  bool hasAttributes = true;
  if (hasAttributes)
  {
    for (glResource &attribute : m_resources)
    {
      if (attribute.rType != 1)
        continue;
      attribute.rType = 2;
    }
  }
}

int main()
{
  Core core;
  core.m_resources.data[0].rType = 1;
  core.Render();
  return core.m_resources.data[0].rType == 2 ? 0 : 1;
}
