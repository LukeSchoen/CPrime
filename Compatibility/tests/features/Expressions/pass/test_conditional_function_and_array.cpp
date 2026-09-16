int first() { return 17; }
int second() { return 29; }
int invoke(int (&function)()) { return function(); }
template<int N> int length(const char (&)[N]) { return N; }
int main() {
  bool condition = true;
  int (&function)() = condition ? first : second;
  if (function() != 17 || invoke(condition ? second : first) != 29) return 1;
  if (length(condition ? "x" : "y") != 2) return 2;
  const char *text = condition ? "" : "x";
  return text[0] == 0 ? 0 : 3;
}
