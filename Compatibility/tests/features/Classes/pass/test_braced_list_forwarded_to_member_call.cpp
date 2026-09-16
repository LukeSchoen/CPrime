template<typename T>
class SimpleList
{
public:
  SimpleList() : count(0) {}
  SimpleList(const T &a, const T &b) : count(2), first(a), second(b) {}
  const T *Data() const { return &first; }
  int Size() const { return count; }

  int count;
  T first;
  T second;
};

class Core
{
public:
  void SetChannel(const void *data, int count);
  const void *lastData;
  int lastCount;
};

void Core::SetChannel(const void *data, int count)
{
  lastData = data;
  lastCount = count;
}

class Render
{
public:
  void SetUniform(const SimpleList<float> &value);
  Core core;
};

void Render::SetUniform(const SimpleList<float> &value)
{
  core.SetChannel(value.Data(), value.Size());
}

int main()
{
  Render render;
  render.SetUniform({ 1.0f, 2.0f });
  return 0;
}
