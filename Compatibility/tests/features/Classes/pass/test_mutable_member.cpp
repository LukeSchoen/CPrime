struct CachedValue
{
  mutable int cache;

  int Get() const
  {
    cache = 17;
    return cache;
  }
};

int main()
{
  CachedValue value;
  return value.Get() == 17 ? 0 : 1;
}
