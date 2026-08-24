template<class T>
T ReadValue(T *value)
{
  return *value;
}

int main()
{
  int value = 23;
  return ReadValue(&value) == 23 ? 0 : 1;
}
