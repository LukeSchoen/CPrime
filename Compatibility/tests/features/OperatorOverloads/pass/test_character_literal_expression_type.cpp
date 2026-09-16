#include <type_traits>

struct Text {
  int selected;
  Text() : selected(0) {}
  Text& operator+=(const char&) { selected = 1; return *this; }
  template<class T> Text& operator+=(const T&) { selected = 2; return *this; }
};

int select(char) { return 1; }
int select(int) { return 2; }
template<class T> int forwarded(T value) { return select(value); }

int main()
{
  static_assert(std::is_same<decltype('x'), char>::value, "ordinary literal is char");
  static_assert(std::is_same<decltype(+'x'), int>::value, "unary plus promotes");
  static_assert(std::is_same<decltype(L'x'), wchar_t>::value, "wide literal is wchar_t");
  Text text;
  text += 'x';
  if (text.selected != 1) return 1;
  text += ('y');
  if (text.selected != 1) return 2;
  if (select('z') != 1 || forwarded('z') != 1 || select(+'z') != 2) return 3;
  return 0;
}
