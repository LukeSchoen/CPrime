template<typename T>
void UnsupportedUnlessSpecialized()
{
  static_assert(false, "unsupported type");
}

int main()
{
  return 0;
}
