// EXPECT_COMPILE_ARGS: -std=c++17
// Recursive index selection over a nested pack: the concrete base case and
// the deduced step must both match, with the concrete pattern ordered as the
// more specialized one.
#include <stddef.h>

template<class... Ts> struct box { };

template<size_t Index, class T> struct element;
template<class A, class... Rest>
struct element<0, box<A, Rest...> > { typedef A type; };
template<size_t Index, class A, class... Rest>
struct element<Index, box<A, Rest...> >
{
  typedef typename element<Index - 1, box<Rest...> >::type type;
};

static_assert(sizeof(typename element<0, box<char, int> >::type) == sizeof(char),
              "first element");
static_assert(sizeof(typename element<1, box<char, int> >::type) == sizeof(int),
              "second element");

int main() { return 0; }
