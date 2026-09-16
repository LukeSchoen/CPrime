// The terminal name of a qualified id in a replayed class-template member body
// belongs to the qualifier in front of it, not to the instantiation being
// replayed.  `typename Outer<N-1>::Inner` inside Outer<N> reaches the
// specialization's Inner; it used to be redirected to Outer<N>'s own joined
// member name, so the qualified lookup searched Outer<1> for the name of
// Outer<2>::Inner and reported `nested template type member ... must be a
// typedef`.

template <int N>
struct Outer
{
  struct Inner
  {
    Inner(int n) : sum(n) {}

    typename Outer<N - 1>::Inner operator[](int n) const
    { return typename Outer<N - 1>::Inner(sum + n); }

    int sum;
  };

  struct Marker { int tag; };

  typename Outer<N - 1>::Inner operator[](int n) const
  { return typename Outer<N - 1>::Inner(n); }

  /* The same nested class written through this instantiation's own
     qualifier still names the member of the instantiation in front of it. */
  Outer<N>::Marker marker() const
  {
    Outer<N>::Marker m;
    m.tag = N;
    return m;
  }
};

template <>
struct Outer<1>
{
  struct Inner
  {
    Inner(int n) : sum(n) {}

    int operator[](int n) const { return sum + n; }

    int sum;
  };

  struct Marker { int tag; };

  int operator[](int n) const { return n; }

  Marker marker() const
  {
    Marker m;
    m.tag = 1;
    return m;
  }
};

/* A member typedef of the class template shares the same terminal name, so
   the alias the replay publishes for it must not capture the qualified id
   either. */
template <int N>
struct Alias
{
  typedef int Inner;

  typename Alias<N - 1>::Inner value() const
  { return typename Alias<N - 1>::Inner(); }
};

template <>
struct Alias<1>
{
  struct Inner { Inner() {} };
};

/* A member function template of the class template replays its body through
   a separate substitution path, so it needs the same rule. */
template <int N>
struct MemberTemplate
{
  template <class T>
  typename Outer<N - 1>::Inner get(T) const
  { return typename Outer<N - 1>::Inner(N); }
};

/* `::template` puts the keyword between the qualifier and the name, and the
   instantiation has a member template of that name of its own. */
template <int N>
struct MemberTemplateBox
{
  template <class T> struct Box { T t; Box() : t(N) {} };

  template <class T>
  typename MemberTemplateBox<N - 1>::template Box<T> make(T) const
  { return typename MemberTemplateBox<N - 1>::template Box<T>(); }
};

template <>
struct MemberTemplateBox<1>
{
  template <class T> struct Box { T t; Box() : t(1) {} };
};

int main()
{
  Outer<1> sum1;
  Outer<2> sum2;
  Outer<3> sum3;
  if (sum1[1] != 1)
    return 1;
  if (sum1.marker().tag != 1)
    return 2;
  if (sum2[1][1] != 2)
    return 3;
  if (sum3[1][1][1] != 3)
    return 4;
  if (sum3.marker().tag != 3)
    return 5;

  Alias<2> alias2;
  typename Alias<1>::Inner inner = alias2.value();
  (void)inner;

  MemberTemplate<2> member;
  if (member.get(1).sum != 2)
    return 6;

  MemberTemplateBox<2> box;
  if (box.make(1).t != 1)
    return 7;

  return sum1[1] + sum2[1][1] - 3;
}
