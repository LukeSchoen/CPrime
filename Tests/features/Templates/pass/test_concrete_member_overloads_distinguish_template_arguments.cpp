template <typename T>
struct OverloadList
{
  T value;
};

struct OverloadVec3
{
  float x, y, z;
};

struct TemplateArgumentOverloads
{
  int selected;

  TemplateArgumentOverloads() : selected(0) {}

  void Set(const char *, const OverloadList<char> &) { selected = 1; }
  void Set(const char *, const OverloadList<OverloadVec3> &) { selected = 3; }
};

int main()
{
  TemplateArgumentOverloads target;
  OverloadList<OverloadVec3> values;
  target.Set("position0", values);
  return target.selected == 3 ? 0 : 1;
}
