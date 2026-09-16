// EXPECT_COMPILE_ARGS: -std=c++17
// A partial specialization whose pattern opens another class template over
// the specialization's own pack (`traits<box<Ts...>>`) must deduce from the
// concrete argument list, including an empty list.
#include <stddef.h>

template<class... Ts> struct box { };

template<class T> struct arity;
template<class... Ts> struct arity<box<Ts...> >
{
  enum { value = sizeof...(Ts) };
};

template<class T> struct head_of;
template<class A, class... Rest> struct head_of<box<A, Rest...> >
{
  typedef A type;
};

static_assert(arity<box<> >::value == 0, "empty pack");
static_assert(arity<box<int> >::value == 1, "single argument");
static_assert(arity<box<int, char, double> >::value == 3, "three arguments");
static_assert(sizeof(typename head_of<box<int, char> >::type) == sizeof(int),
              "leading argument");

int main() { return 0; }
