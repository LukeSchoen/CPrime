// A class left operand of `->*` names an overloaded operator resolved by
// argument-dependent lookup, not just the built-in class-pointer form.
struct A {
  int m_value;
  friend int operator->* (A &object, int A::*) {
    return object.m_value;
  }
};

int A::*member = &A::m_value;

int main() {
  A object;
  object.m_value = 42;
  if ((object ->* member) != 42) return 1;
  return 0;
}
