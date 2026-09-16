template<typename T>
struct TraitLikeTypeProbe
{
  static const bool value = false;
};

typedef TraitLikeTypeProbe<int> IntTraitLikeTypeProbe;

int main()
{
  return 0;
}
