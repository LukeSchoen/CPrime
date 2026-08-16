template<typename T, typename... Args> void first(T *p, Args&&... args)
{
  new(p) T(args...);
}

template<typename T> void second(T *p)
{
  (void)p;
}

int main()
{
  int value;
  first(&value, 3);
  second(&value);
  return value == 3 ? 0 : 1;
}
