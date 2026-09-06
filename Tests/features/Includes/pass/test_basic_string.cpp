#include <iosfwd>
#include <string>
#include <type_traits>
static_assert(std::is_same<std::string, std::basic_string<char> >::value, "string alias");
int main() {
  std::string a("abcd");
  a.append(a.data() + 1, 3);
  if (a.size() != 7 || a[4] != 'b' || a[6] != 'd') return 1;
  a.assign(a.data() + 2, 4);
  if (a.size() != 4 || a[0] != 'c' || a[3] != 'c') return 2;
  std::string b = a;
  a.resize(8, 'x');
  if (b.size() != 4 || a[7] != 'x' || a.c_str()[8]) return 3;
  std::wstring w(L"wide");
  w += L"!";
  if (w.size() != 5 || w.substr(1, 2)[0] != L'i') return 4;
  std::string empty;
  if (!empty.data() || empty.c_str()[0]) return 5;
  return 0;
}
