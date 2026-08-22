typedef unsigned char ui8;

int main()
{
  if (!std::is_floating_point<float>::value) return 1;
  if (std::is_floating_point<int>::value) return 2;
  if (!std::is_signed<int>::value) return 3;
  if (!std::is_unsigned<ui8>::value) return 4;
  if (std::is_unsigned<int>::value) return 5;
  return 0;
}
