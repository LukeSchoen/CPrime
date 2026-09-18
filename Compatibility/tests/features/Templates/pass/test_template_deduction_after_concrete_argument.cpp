/* A function template parameter pattern may mix deduced parameters with
   concrete arguments in any order.  Boost.ContainerHash declares
   `hash_value(std::basic_string<Ch, traits<Ch>, A> const&)`, whose last
   parameter follows a nested template-id; every written argument, concrete or
   not, has to advance the position that maps a later parameter onto the
   class's argument list, or the trailing parameter is left unbound and the
   fallback binds the whole argument to it. */
#include <string>

template <class T> struct traits_local {};

template <class C, class Derived, class A> struct wrap {};

template <class C, class A> int pick(wrap<C, traits_local<C>, A> const&)
{
  return sizeof(C) == sizeof(char) && sizeof(A) == sizeof(int) ? 0 : 1;
}

template <class C, class A> int pick_plain(wrap<C, traits_local<char>, A> const&)
{
  return sizeof(C) == sizeof(char) && sizeof(A) == sizeof(int) ? 0 : 2;
}

template <class Ch, class A> unsigned long hash_like(
    std::basic_string<Ch, std::char_traits<Ch>, A> const& value)
{
  return value.size();
}

int main()
{
  wrap<char, traits_local<char>, int> dependent;
  wrap<char, traits_local<char>, int> plain;
  std::string text("abc");
  if (pick(dependent)) return 3;
  if (pick_plain(plain)) return 4;
  if (hash_like(text) != 3) return 5;
  return 0;
}
