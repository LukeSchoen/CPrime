/* A member function template parameter names a nested class template that
   takes a pointer-to-member value.  An address of a static member cannot form
   that parameter type, so the candidate is dropped by substitution and the
   ellipsis overload is selected; a non-static member's address still
   matches. */

struct small_type { char dummy; };
struct large_type { char dummy[2]; };

template<class T>
struct has_foo_member_variable
{
  template<int T::*> struct tester;
  template<class U> static small_type has_foo (tester<&U::foo> *);
  template<class U> static large_type has_foo (...);
  static const bool value = (sizeof (has_foo<T> (0)) == sizeof (small_type));
};

struct function_foo
{
  static int foo () { return 0; }
};

struct static_data_foo
{
  static int foo;
};

int static_data_foo::foo = 0;

struct member_foo
{
  int foo;
};

int main ()
{
  if (has_foo_member_variable<function_foo>::value) return 1;
  if (has_foo_member_variable<static_data_foo>::value) return 2;
  if (!has_foo_member_variable<member_foo>::value) return 3;
  return 0;
}
