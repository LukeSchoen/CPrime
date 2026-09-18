/* A pack expansion inside a function type names a parameter list, so the
   expanded parameters may start with any type.  The first expansion element
   used to be read as the whole list, which rejected a leading reference,
   pointer or const-qualified type with "')' expected (got ',')".  The shape
   spells the handler as `std::function<bool(Args...)>`; the bare alias
   spelling and the partial-specialisation pattern of the same form are
   covered here too. */
// EXPECT_EXIT: 0
#include <functional>

bool target_function(int &value, float) { value += 1; return true; }

template <class... Args>
class EventSource {
public:
  using Handler = std::function<bool(Args...)>;
  Handler handler;
};

/* The same expansion written directly on a class-scope alias: the template
   replay declares the alias as `bool Handler(Args...)`, not as the ill-formed
   `bool(Args...) Handler`. */
template <class... Args>
struct direct_event {
  using Handler = bool(Args...);
  Handler *handler;
};

/* `...` after the pack stays a C-style ellipsis in the expanded list. */
template <class... Args>
struct variadic_event {
  using Handler = bool(Args..., ...);
  Handler *handler;
};

struct methods {
  int field;
  bool task(int &, float);
};

/* The partial-specialisation pattern that std::function itself relies on. */
template <class> struct signature;
template <class R, class... A> struct signature<R(A...)> { enum { matched = 1 }; };

int main() {
  EventSource<int &, float> reference_first;
  EventSource<int &&, float> rvalue_reference_first;
  EventSource<int const &, float> const_reference_first;
  EventSource<int *, float> pointer_first;
  EventSource<void (*)(int), float> function_pointer_first;
  EventSource<int (&)[3], float> array_reference_first;
  EventSource<int, float> plain;
  EventSource<int, float &> reference_second;
  EventSource<int, float, double> three;
  EventSource<int &> single;
  EventSource<> empty;
  EventSource<int &, float, double> reference_then_two;

  direct_event<int &, float> direct;
  direct_event<int methods::*, float> direct_member_pointer;
  direct_event<bool (methods::*)(int &, float), float> direct_member_function;
  direct_event<int (&)[3], float> direct_array_reference;
  variadic_event<int &> trailing_ellipsis;
  signature<bool(int &, float)> pattern;

  int value = 1;
  reference_first.handler = &target_function;
  if (!reference_first.handler(value, 1.0f)) return 1;
  if (value != 2) return 2;
  direct.handler = 0;
  direct_member_pointer.handler = 0;
  direct_member_function.handler = 0;
  direct_array_reference.handler = 0;
  trailing_ellipsis.handler = 0;
  if (pattern.matched != 1) return 3;
  if (sizeof(rvalue_reference_first) == 0) return 4;
  if (sizeof(const_reference_first) == 0) return 5;
  if (sizeof(pointer_first) == 0) return 6;
  if (sizeof(function_pointer_first) == 0) return 7;
  if (sizeof(direct_member_pointer) == 0) return 8;
  if (sizeof(direct_member_function) == 0) return 9;
  if (sizeof(array_reference_first) == 0 || sizeof(direct_array_reference) == 0) return 10;
  if (sizeof(plain) == 0 || sizeof(reference_second) == 0) return 11;
  if (sizeof(three) == 0 || sizeof(single) == 0) return 12;
  if (sizeof(empty) == 0 || sizeof(reference_then_two) == 0) return 13;
  return 0;
}
