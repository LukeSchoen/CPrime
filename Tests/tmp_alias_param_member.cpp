template <typename T>
struct LowerVec
{
  T x;
  T y;
};

typedef LowerVec<float> v2_alias_probe;

template <typename T>
struct AliasParamBox
{
  AliasParamBox() = default;
  AliasParamBox(const T &value) : min(value), max(value) {}
  v2_alias_probe Map(v2_alias_probe pos);
  T min;
  T max;
};

int main()
{
  AliasParamBox<float> box(1.0f);
  (void)box;
  return 0;
}
