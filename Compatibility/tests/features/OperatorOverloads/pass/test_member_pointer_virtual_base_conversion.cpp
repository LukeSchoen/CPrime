// EXPECT_EXIT: 0
// [conv.mem]: a pointer to member of a base class converts to a pointer to
// member of a derived class only through a non-virtual base, so the virtual
// base conversion must not be viable.

struct Base {
  int member(int value) { return value; }
};
struct VirtualDerived : virtual Base {
};
struct PlainDerived : Base {
};

template <bool, typename T = void> struct enable_if {
  typedef T type;
};
template <typename T> struct enable_if<false, T> {
};

template <typename From, typename To> struct converts {
  typedef char one;
  typedef struct { char storage[2]; } two;
  template <typename T> static void sink(T);
  template <typename To1, typename From1>
    static typename enable_if<(sizeof(sink<To1>(From1()), 1) > 0), one>::type
    test(int);
  template <typename, typename> static two test(...);
  static const bool value = sizeof(test<To, From>(0)) == 1;
};

int main() {
  if (!converts<int (Base::*)(int), int (PlainDerived::*)(int)>::value) return 1;
  if (converts<int (Base::*)(int), int (VirtualDerived::*)(int)>::value) return 2;
  return 0;
}
