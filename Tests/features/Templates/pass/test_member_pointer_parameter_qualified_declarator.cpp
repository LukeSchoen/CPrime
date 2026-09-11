// A value template parameter may have pointer-to-member type with a named,
// cv-qualified declarator: `void (T::* const U)()`.  The abstract-declarator
// parse used for the parameter type stopped after the cv qualifier and left
// the name for the enclosing `)`, so a valid member address argument was
// rejected as not matching the parameter.

struct A {
  void method() {}
  int field;
};

template <class T, void (T::* const U)()> struct function_slot {
  static int const value = 1;
};

template <class T, int T::* const U> struct field_slot {
  static int const value = 2;
};

int main() {
  if (function_slot<A, &A::method>::value != 1) return 1;
  if (field_slot<A, &A::field>::value != 2) return 2;
  return 0;
}
