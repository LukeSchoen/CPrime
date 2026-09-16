struct Text {
  int selected;
  Text(const char*) : selected(1) {}
  template<class T> explicit Text(const T&) : selected(2) {}
};
int pointer_or_bool(const char*) { return 1; }
int pointer_or_bool(bool) { return 2; }
int main()
{
  Text cast_zero((char)0), literal_zero('\0'), bool_zero(false);
  if (cast_zero.selected != 2 || literal_zero.selected != 2 || bool_zero.selected != 2) return 1;
  if (pointer_or_bool((char)0) != 2 || pointer_or_bool('\0') != 2) return 2;
  if (pointer_or_bool(nullptr) != 1) return 3;
  return 0;
}
