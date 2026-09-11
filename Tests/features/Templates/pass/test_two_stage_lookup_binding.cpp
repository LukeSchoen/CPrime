/* Unqualified non-dependent names in a template body bind where the template
   was written: a later overload does not win, and a member of a base that was
   dependent at the definition cannot hide an enclosing-scope name. */

int g(double) { return 0; }
int foo() { return 0; }

class Base {
public:
  int foo() { return 1; }
};

template<class T>
class Derived : public T {
public:
  int CallFoo() { return foo(); }
};

template<class T>
struct LateCall {
  int CallG() { return g(2); }
};

inline int g(int) { return 1; }

int main()
{
  Derived<Base> derived;
  LateCall<int> late;
  if (derived.CallFoo() != 0)
    return 1;
  if (late.CallG() != 0)
    return 2;
  return 0;
}
