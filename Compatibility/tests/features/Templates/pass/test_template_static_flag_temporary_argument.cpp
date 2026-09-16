namespace flags { typedef unsigned kind; static const kind value = 1; }
template<class T> struct Value {
  typedef flags::kind flag_type;
  static const flag_type flag = flags::value;
  flag_type stored;
  explicit Value(const T *, flag_type input = flag) : stored(input) {}
};
template<class T> bool read(const T*, const Value<T>& value) { return value.stored == 1; }
int main() { return !read("x", Value<char>("x", Value<char>::flag)) || !read(L"x", Value<wchar_t>(L"x")); }
