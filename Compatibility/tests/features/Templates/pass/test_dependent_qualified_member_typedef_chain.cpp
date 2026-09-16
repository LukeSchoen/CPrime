/* The terminal name of `T::Alias::Member` is qualified by the class the
   alias selects, not by the specialization whose body is being replayed.
   An enclosing-class member of the same name must not capture it. */

template<class T> struct Holder;

struct Leaf
{
  int marker;
  typedef int Value;
  typedef Holder<Leaf> Alias;
};

template<class T>
struct Holder
{
  typedef Leaf Value;
  typename T::Alias::Value member;
};

struct Wrapper { typedef Holder<Leaf> Alias; };

int main ()
{
  Holder<Wrapper> holder;
  holder.member.marker = 7;
  return holder.member.marker == 7 ? 0 : 1;
}
