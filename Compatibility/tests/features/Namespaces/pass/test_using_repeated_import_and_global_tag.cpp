namespace first
{
template <typename T> int equal(T, int) { return 1; }
}

namespace second
{
template <typename T> int equal(T, double) { return 2; }
}

namespace counts
{
int exact() { return 7; }
}

namespace fallback
{
template <typename T> int exact(T, long) { return 8; }
}

template <typename T> int merged_templates(T value)
{
  using first::equal;
  using second::equal;
  return equal(value, 1);
}

int merged_function_and_template()
{
  using counts::exact;
  using fallback::exact;
  return exact();
}

struct Tag
{
  int value;
};

int main()
{
  if (merged_templates(4) != 1)
    return 1;
  if (merged_function_and_template() != 7)
    return 2;

  int Tag = 3;
  using ::Tag;
  struct Tag tag;
  tag.value = Tag;
  return tag.value == 3 ? 0 : 3;
}
