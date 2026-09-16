#include <string>

int main()
{
  std::wstring text(7, L'\0');
  const wchar_t source[] = L"Assets/";
  for (int i = 0; i < 7; ++i)
    text.data()[i] = source[i];
  return text.length() == 7
      && text.c_str()[0] == L'A'
      && text.c_str()[6] == L'/'
      && text.c_str()[7] == L'\0'
      ? 0 : 1;
}
