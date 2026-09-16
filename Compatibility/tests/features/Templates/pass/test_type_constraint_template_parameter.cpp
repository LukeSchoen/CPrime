// A template parameter written as a type-constraint (`template <C T>`) names a
// type parameter constrained by the concept C.  The constraint is not
// evaluated here, so `C T` must read as the type parameter it constrains
// rather than as a value parameter whose type is C.

template <typename T>
concept addable = requires(T &t) { t + 0; };

template <addable T>
struct Holder
{
  T held;
};

template <addable T>
T twice (T value)
{
  return value + value;
}

template <addable T, typename U>
struct Pair
{
  T first;
  U second;
};

template <typename T>
int width ()
{
  return (int) sizeof (T);
}

namespace nested
{
template <addable T>
using alias_of = Holder<T>;
}

int main ()
{
  Holder<int> holder = {7};

  if (holder.held != 7)
    return 1;
  if (twice (3) != 6)
    return 2;

  Pair<int, double> pair = {2, 0.5};

  if (pair.first != 2 || pair.second != 0.5)
    return 3;

  nested::alias_of<int> alias = {4};

  if (alias.held != 4)
    return 4;
  if (width<Holder<int> > () != (int) sizeof (int))
    return 5;
  return 0;
}
