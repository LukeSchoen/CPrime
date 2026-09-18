/* std::basic_string_view carries the standard traits parameter, so code that
   spells both arguments - a Boost.ContainerHash value hash overload, for one -
   both names the type and deduces the traits from it. */
#include <string_view>

typedef std::basic_string_view<char, std::char_traits<char> > explicit_view;

template <class Ch> unsigned long measure(
    std::basic_string_view<Ch, std::char_traits<Ch> > const& value)
{
  return value.size();
}

int main()
{
  std::string_view view("hello");
  explicit_view other("world!");
  if (view.size() != 5) return 1;
  if (other.size() != 6) return 2;
  if (measure(view) != 5) return 3;
  if (measure(other) != 6) return 4;
  /* The traits type stays usable through the class. */
  if (explicit_view::traits_type::length("abc") != 3) return 5;
  return 0;
}
