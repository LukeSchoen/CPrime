// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <string>

int main()
{
  const char narrow[] = {'a', 0, 'b', 'c'};
  const wchar_t wide[] = {L'd', 0, L'e', L'f'};
  std::string s(narrow, 3);
  std::wstring w(wide, 3);
  if (s.length() != 3 || s[0] != 'a' || s[1] != 0 || s[2] != 'b' || s.c_str()[3] != 0)
    return 1;
  if (w.length() != 3 || w[0] != L'd' || w[1] != 0 || w[2] != L'e' || w.c_str()[3] != 0)
    return 2;
  std::string empty(narrow, 0);
  std::wstring wempty(wide, 0);
  if (!empty.empty() || empty.c_str()[0] || !wempty.empty() || wempty.c_str()[0])
    return 3;
  std::string repeated(3, 'g');
  std::wstring wrepeated(3, L'h');
  return repeated[2] != 'g' || wrepeated[2] != L'h';
}
