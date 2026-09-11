/* Two-stage lookup: an unqualified call in a template body resolves against
   the declarations visible where the template was written.  A function
   template visible there is instantiated for the call, so an overload of the
   same name declared later in the same namespace is not a candidate even
   when it is an exact match for the argument. */

namespace N
{
template<class T> char probe (T);

template<class T> struct Check
{
  static T const sample;
  enum { value = 1 == sizeof (probe (sample)) };
};

double probe (int);

struct Item { Item (int v = 0) : value_ (v) {} int value_; };

template<class T> T build (T) { return T (); }
template<class T> T relay (T item) { return build (item); }

Item build (Item) { return Item (1); }
}

int main ()
{
  if (!N::Check<int>::value)
    return 1;
  if (N::relay (N::Item ()).value_ != 0)
    return 2;
  return 0;
}
