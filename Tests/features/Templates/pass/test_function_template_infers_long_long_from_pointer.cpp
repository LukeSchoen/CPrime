template<typename T>
void destroy(T *value)
{
}

int main()
{
  long long value = 0;
  destroy(&value);
  return 0;
}
