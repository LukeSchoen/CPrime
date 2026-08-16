template<typename T>
void destroy(T *value)
{
}

int main()
{
  char value = 0;
  destroy(&value);
  return 0;
}
