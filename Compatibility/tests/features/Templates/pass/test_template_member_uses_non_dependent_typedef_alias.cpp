template <typename T>
struct AliasVec
{
  T x;
  T y;
};

typedef AliasVec<float> alias_v2;

template <typename T>
struct AliasBox
{
  AliasBox() = default;
  AliasBox(const T &value) : min(value), max(value) {}

  alias_v2 Map(alias_v2 pos);

  T min;
  T max;
};

template <typename T>
alias_v2 AliasBox<T>::Map(alias_v2 pos)
{
  return pos;
}

int main()
{
  AliasBox<float> box(1.0f);
  alias_v2 pos = { 1.0f, 2.0f };
  alias_v2 mapped = box.Map(pos);
  (void)mapped;
  return 0;
}
