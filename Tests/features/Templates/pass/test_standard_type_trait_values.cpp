int main()
{
  if (!std::is_floating_point<float>::value) return 1;
  if (std::is_floating_point<int>::value) return 2;
  if (!std::is_signed<int>::value) return 3;
  return 0;
}
