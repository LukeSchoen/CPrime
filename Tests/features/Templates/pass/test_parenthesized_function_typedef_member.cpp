// A class-template member typedef may name a function type through a
// parenthesized declarator.  `typedef Y (FP) ();` declares FP and uses Y as
// its return type, so instantiating the class must publish FP itself; reading
// the identifier in front of the group as the declared name made the replayed
// member a reference to an undeclared typedef.  The dependent default
// template argument of B and the incomplete A preserve the relevant shape.

template <typename> struct A;
template <typename T, typename = A<T> > struct B { };
template <typename U> struct Complete { };

template <class W, class>
struct D
{
  typedef W X;
  typedef X (FP) ();
  Complete<FP &> *a;
};

template <class Y>
struct E
{
  typedef Y (FP) ();
  B<FP &> b;
};

static int thrice ()
{
  return 3;
}

int main ()
{
  E<int> e;
  E<int>::FP *p = thrice;
  D<int, char> d;
  D<int, char>::FP *q = thrice;

  (void) e;
  (void) d;
  if (p() != 3)
    return 1;
  if (q() != 3)
    return 2;
  return 0;
}
