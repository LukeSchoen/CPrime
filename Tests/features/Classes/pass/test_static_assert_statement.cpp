static_assert(sizeof(int) >= 4, "int must be at least 32 bits");

int main()
{
  static_assert(sizeof(char) == 1);
  return 0;
}
