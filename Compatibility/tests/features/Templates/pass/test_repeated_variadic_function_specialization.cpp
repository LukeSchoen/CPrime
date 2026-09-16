template<typename T, typename... Args>
void Construct(T *target, Args&&... args)
{
  new(target) T(args...);
}

int main()
{
  int first = 0;
  int second = 0;
  Construct(&first, 7);
  Construct(&second, 11);
  return first == 7 && second == 11 ? 0 : 1;
}
