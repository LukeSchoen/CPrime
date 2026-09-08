struct Format {
  template<class T> int operator%(const T&) { return 1; }
  template<class T> int operator%(T&) { return 2; }
};
template<class T> int check(Format &format, bool condition) {
  if (format % (condition ? "x" : "y") != 1) return 1;
  if (format % (condition ? "" : "x") != 1) return 2;
  return 0;
}
int main() {
  Format format;
  bool condition = true;
  int value = 7;
  if (format % value != 2) return 1;
  if (format % (condition ? "x" : "y") != 1) return 2;
  if (format % (condition ? "" : "x") != 1) return 3;
  if (check<int>(format, condition)) return 4;
  return format % value == 2 ? 0 : 5;
}
