struct Handle {
  typedef void (*safe_bool)(Handle***);
  bool live;
  static void marker(Handle***) {}
  operator safe_bool() const;
};
Handle::operator Handle::safe_bool() const { return live ? &marker : 0; }
int main() {
  Handle yes = {true}, no = {false};
  if (!yes || no) return 1;
  int count = 0;
  while (yes) { ++count; yes.live = false; }
  return count != 1;
}
