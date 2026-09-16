template<typename T>
struct TraitLikeLocal
{
  static const bool value = false;
};

int main()
{
  if (TraitLikeLocal<int>::value)
    return 1;
  return 0;
}
