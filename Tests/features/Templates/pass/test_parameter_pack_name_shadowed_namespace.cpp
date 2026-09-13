// A namespace and a function parameter pack can share one spelling.  A name
// followed by `::` is looked up as a namespace or a class, and a name after
// `.` or `->` is a member, so replaying the body must keep those spellings
// while a real use of the pack still expands element-wise.

namespace ranges
{
  int value = 3;

  template <class T>
  struct Wrap
  {
    T held;
  };
}

struct Holder
{
  int ranges;
};

template <class... Ranges>
int measure (Holder &holder, Ranges &&...ranges)
{
  int total = ranges::value;
  total += (int) sizeof (ranges::Wrap<int>);
  total += holder.ranges;
  ((total += ranges), ...);
  return total;
}

template <class... Ranges>
int measure_pointer (Holder *holder, Ranges &&...ranges)
{
  int total = holder->ranges;
  ((total += ranges), ...);
  return total;
}

int main ()
{
  Holder holder = {5};

  if (measure (holder, 1, 2) != 15)
    return 1;
  if (measure_pointer (&holder, 4) != 9)
    return 2;
  return 0;
}
