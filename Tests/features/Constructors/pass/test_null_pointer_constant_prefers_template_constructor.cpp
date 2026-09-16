// A literal zero argument is a null pointer constant, but binding it to a
// constructor template's exact parameter type is a reference binding, which
// beats the pointer conversions of the ordinary constructors. Selecting the
// pointer constructors instead made the call ambiguous.

struct Text
{
  Text(const char *text) : kind(1) { (void)text; }
  Text(const wchar_t *text) : kind(2) { (void)text; }
  template<typename T> explicit Text(const T &value) : kind(3) { (void)value; }
  int kind;
};

int main()
{
  Text zero(0);
  if (zero.kind != 3) return 1;
  Text number(7);
  if (number.kind != 3) return 2;
  Text narrow("text");
  if (narrow.kind != 1) return 3;
  return 0;
}
