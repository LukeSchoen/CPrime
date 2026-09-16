// The GCC __has_trivial_* / __has_nothrow_* spellings answer as type traits
// instead of being parsed as calls to undeclared functions.  References have
// trivial special members, other non-class operands do not, and a class
// answers for the implicit special member its data members keep trivial.

struct Trivial {
  int x;
};

struct WithDestructor {
  ~WithDestructor ();
};

typedef char assert_reference [__has_trivial_destructor (int&) ? 1 : -1];
typedef char assert_scalar [__has_trivial_destructor (int) ? -1 : 1];
typedef char assert_pointer [__has_trivial_destructor (int *) ? -1 : 1];
typedef char assert_trivial_class [__has_trivial_destructor (Trivial) ? 1 : -1];
typedef char assert_non_trivial_class [__has_trivial_destructor (WithDestructor) ? -1 : 1];
typedef char assert_trivial_constructor [__has_trivial_constructor (Trivial) ? 1 : -1];
typedef char assert_nothrow_copy [__has_nothrow_copy (Trivial) ? 1 : -1];

int main ()
{
  Trivial value = { 7 };
  return value.x == 7 ? 0 : 1;
}
