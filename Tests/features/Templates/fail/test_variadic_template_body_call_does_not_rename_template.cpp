template<typename T, typename... Args> void construct(T *p, Args&&... args)
{
  new(p) T(args...);
}

template<typename T, typename... Args> void constructArray(T *p, int count, Args&&... args)
{
  for (int i = 0; i < count; ++i)
    construct(p + i, args...);
}

int main()
{
  int values[2];
  constructArray(values, 2, 7);
  return values[0] == 7 && values[1] == 7 ? 0 : 1;
}
