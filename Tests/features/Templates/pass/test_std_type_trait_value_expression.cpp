int main()
{
  if (std::is_trivially_copyable<int>::value)
    return 1;
  return 0;
}
