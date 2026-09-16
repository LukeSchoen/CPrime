// A using-directive makes the nested namespace `std::ranges` visible by its
// own name, so `ranges::...` after `using namespace std;` names that
// namespace, not a namespace spelled `ranges` beside it.

namespace std
{
namespace ranges
{
  int value = 4;

  template <class T>
  struct Wrap
  {
    T held;
  };
}
}

using namespace std;

template <class... Ranges>
int counted (Ranges &&...ranges)
{
  ranges::Wrap<int> wrap = {ranges::value};
  ((wrap.held += (int) sizeof (ranges)), ...);
  return wrap.held;
}

int main ()
{
  if (ranges::value != 4)
    return 1;

  ranges::Wrap<int> wrap = {7};
  if (wrap.held != 7)
    return 2;
  if (counted (1, 2) != 12)
    return 3;
  return 0;
}
