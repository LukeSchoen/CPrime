template<typename T, typename... Args>
void ConstructOne(T *target, Args&&... args)
{
  *target = args;
}

int main()
{
  int value = 0;
  ConstructOne(&value, 42);
  return value == 42 ? 0 : 1;
}
