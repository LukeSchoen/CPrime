/* A base clause that names a member type of a class-template specialization
   has to instantiate the qualifier completely, even when the declaration that
   demands the base is only a type alias.  Boost.MPL writes
   `struct eval_if : if_<C,F1,F2>::type`, and Boost.Variant instantiates it
   from a member typedef while the alias deferral is active; leaving the
   qualifier as a forward declaration used to make the base clause
   unresolvable (`base class type expected`). */

template <class T> struct identity_local
{
  typedef T type;
};

struct tag_zero
{
  enum { value = 0 };
};

struct tag_one
{
  enum { value = 1 };
};

template <bool B, class T1, class T2> struct if_c_local
{
  typedef T1 type;
};

template <class T1, class T2> struct if_c_local<false, T1, T2>
{
  typedef T2 type;
};

template <class T1, class T2, class T3> struct if_local
{
private:
  typedef if_c_local<static_cast<bool>(T1::value), T2, T3> almost_type_;
public:
  typedef typename almost_type_::type type;
};

template <class C, class F1, class F2>
struct eval_if_local : if_local<C, F1, F2>::type
{
};

template <class T> struct selector
{
  typedef typename eval_if_local<tag_zero, identity_local<T>,
                                 identity_local<int> >::type selected;
};

template <class T> struct selector_taken
{
  typedef typename eval_if_local<tag_one, identity_local<T>,
                                 identity_local<int> >::type selected;
};

int main()
{
  /* The condition picks identity_local<int> for tag_zero and
     identity_local<double> for tag_one, so the inherited `type` names the
     matching argument. */
  if (sizeof(selector<double>::selected) != sizeof(int)) return 1;
  if (sizeof(selector_taken<double>::selected) != sizeof(double)) return 2;
  return 0;
}
