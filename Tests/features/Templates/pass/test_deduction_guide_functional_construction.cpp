// A written deduction guide infers a class template's argument from the
// functional spelling `Class(argument)`: the guide's parameter deduces the
// class's argument and the guide's result type names the instantiated class,
// whose aggregate member takes the argument list.

namespace lib
{
template <typename T>
concept addable = requires(T &t) { t + 0; };

template <addable R>
struct elements_of
{
  R range;
};

template <typename R>
elements_of (R &&) -> elements_of<R &&>;

template <typename R>
struct sized
{
  R held;
};

template <typename R>
sized (R) -> sized<R>;

template <typename R>
int measure (elements_of<R> element)
{
  return (int) element.range;
}

template <typename R>
int measure_value (sized<R> value)
{
  return (int) value.held;
}
}

int main ()
{
  int lvalue = 3;

  /* An lvalue argument collapses the guide's `R &&` parameter to `R &`, so
     the temporary's member refers to the written object. */
  if (lib::measure (lib::elements_of (lvalue)) != 3)
    return 1;

  auto held = lib::elements_of (lvalue);

  held.range = 9;
  if (lvalue != 9)
    return 2;

  /* An rvalue argument binds the reference member to the argument. */
  if (lib::measure (lib::elements_of (4)) != 4)
    return 3;

  /* A guide that names the deduced argument by value. */
  if (lib::measure_value (lib::sized (5)) != 5)
    return 4;
  return 0;
}
