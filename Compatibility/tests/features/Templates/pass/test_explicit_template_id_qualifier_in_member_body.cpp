/* The injected class name denotes the enclosing specialization, but an
   explicit template-id spelled with the class template's own name names that
   other specialization instead.  Inside S<char>::foo the qualifier S<int>
   must still reach S<int>::bar, not the enclosing S<char>. */
template <class T>
struct S
{
  int foo () { return S<int>::bar () + bar (); }

  static int bar () { return sizeof (T) == sizeof (int) ? 3 : 7; }
};

int main ()
{
  S<char> c;
  S<int> i;
  if (c.foo () != 10)
    return 1;
  if (i.foo () != 6)
    return 2;
  return S<int>::bar () == 3 ? 0 : 3;
}
