template<typename T>
int ReadValue(const T *value)
{
  return (int)*value;
}

template<typename T>
int ForwardRead(const T *pTarget)
{
  return ReadValue(pTarget);
}

int main()
{
  int value = 19;
  return ForwardRead(&value) == 19 ? 0 : 1;
}
