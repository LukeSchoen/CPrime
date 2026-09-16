// EXPECT_COMPILE_ARGS: -Werror
int select(int&) { return 1; }
int select(const int&) { return 2; }
int select(int&&) { return 3; }
template<class Element> struct Forwarder {
  template<class... Args> int call(Args&&... args);
};
template<class Element> template<class... Args>
int Forwarder<Element>::call(Args&&... args) {
  return select(static_cast<Args&&>(args)...);
}
struct InlineForwarder {
  template<class... Args> int call(Args&&... args) {
    return select(static_cast<Args&&>(args)...);
  }
};
int main() {
  Forwarder<int> forwarder;
  InlineForwarder inline_forwarder;
  const int fixed = 7;
  int value = 8;
  if (forwarder.call(fixed) != 2) return 1;
  if (forwarder.call(value) != 1) return 2;
  if (forwarder.call(9) != 3) return 3;
  if (forwarder.call(value) != 1 || forwarder.call(fixed) != 2) return 4;
  if (inline_forwarder.call(fixed) != 2 || inline_forwarder.call(value) != 1)
    return 5;
  return inline_forwarder.call(9) != 3;
}
