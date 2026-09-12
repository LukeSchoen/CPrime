// GCC trait builtins consolidated from the retired external corpus.
// Each namespace preserves one distinct incomplete-type or argument-order case.

namespace has_nothrow_assign_default
{
template <bool = __has_nothrow_assign(void)> struct Probe {};
Probe<> probe;
}

namespace trivial_destructor_reference
{
typedef char probe[__has_trivial_destructor(int &) ? 1 : -1];
}

namespace incomplete_pod
{
template <typename T> struct Holder
{
  Holder() {}
};

int probe[__is_pod(Holder<int>) ? -1 : 1];
}

namespace trivially_constructible_ignores_unused_body
{
template <typename T> struct BrokenCtor
{
  BrokenCtor() { T::missing; }
};

int probe = __is_trivially_constructible(BrokenCtor<int>);
}

namespace base_of_uninstantiable
{
template <typename T> struct NonInstantiable
{
  typedef typename T::Missing type;
};

int probe[__is_base_of(NonInstantiable<int>, void) ? -1 : 1];
}

namespace base_of_incomplete_self
{
struct Incomplete;

int same[__is_base_of(Incomplete, Incomplete) ? 1 : -1];
int with_const[__is_base_of(Incomplete, const Incomplete) ? 1 : -1];
int with_volatile[__is_base_of(volatile Incomplete, Incomplete) ? 1 : -1];
}

namespace constructible_argument_order
{
struct A {};
struct B {};
struct C
{
  C(A, B);
};

extern int probe[true];
extern int probe[__is_constructible(C, A, B)];
extern int probe[!__is_constructible(C, B, A)];
}

namespace trivially_constructible_aggregate
{
struct A
{
  int x;
};

struct B
{
  float y;
};

struct C
{
  char z;
};

struct D
{
  A a;
  B b;
  C c;
};

extern int probe[1 + __is_trivially_constructible(D, A)];
extern int probe[1 + __is_trivially_constructible(D, A, B)];
extern int probe[1 + __is_trivially_constructible(D, A, B, C)];
}

namespace underlying_type
{
enum Empty
{
};

enum Signed
{
  minus = -1,
  plus = 1
};

enum Wide
{
  limit = 0x7fffffffffffffff
};

__underlying_type(Empty) empty = 0;
__underlying_type(Signed) signed_value = plus;
__underlying_type(Wide) wide_value = 0x7fffffffffffffff;
}

int main()
{
  return 0;
}
