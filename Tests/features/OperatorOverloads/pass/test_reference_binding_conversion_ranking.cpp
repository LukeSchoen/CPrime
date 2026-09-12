// EXPECT_EXIT: 0
// A direct reference binding is an exact conversion sequence, so a by-value
// candidate must not outrank it before partial ordering. Two reference
// bindings whose referent types agree modulo top-level cv-qualifiers prefer
// the less cv-qualified referent.
template<class T> int pointer(T *) { return 1; }
template<class T> int pointer(T &) { return 2; }
template<class T> int pointer(T const &) { return 3; }

template<class T> int nested(T const *const &) { return 4; }
template<class T> int nested(T *const &) { return 5; }
template<class T> int nested(T *) { return 6; }

template<class T> int qualified(T *const &) { return 7; }
template<class T> int qualified(T const &) { return 8; }

struct Base { int value; };
struct Derived : Base {};
int converted(Base &) { return 9; }
int converted(Base const &) { return 10; }

int main()
{
  int item = 0;
  int const *to_constant = &item;
  int *to_item = &item;
  if (pointer(to_constant) != 1 || pointer(to_item) != 1) return 1;
  if (pointer(*to_constant) != 3 || pointer(*to_item) != 2) return 2;
  if (nested(to_constant) != 4) return 3;
  if (qualified(to_constant) != 7 || qualified(to_item) != 7) return 4;
  Derived derived;
  Base const &constant_base = derived;
  if (converted(derived) != 9 || converted(constant_base) != 10) return 5;
  return 0;
}
