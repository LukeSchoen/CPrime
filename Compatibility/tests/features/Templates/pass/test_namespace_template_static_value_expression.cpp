namespace std
{
  template<class T>
  struct TraitLike
  {
    static const bool value = false;
  };
}

int main()
{
  if (std::TraitLike<int>::value)
    return 1;
  return 0;
}
