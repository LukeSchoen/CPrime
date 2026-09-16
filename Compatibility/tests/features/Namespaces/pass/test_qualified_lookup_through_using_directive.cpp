// Qualified lookup into a namespace also names what that namespace inherits
// through the using-directives declared in it: `tuple::less` and
// `tuple::tag` are `adaptor::less` and `adaptor::tag` here.  Reduced from
// boost's `boost::less_than_comparable1`, which namespace boost imports from
// boost::operators_impl with a using-directive and then names qualified in a
// base-clause.
namespace adaptor
{
  template<class T> struct tag {};
  template<class T> struct less {};
}

namespace tuple { using namespace adaptor; }

template<class T> class pair : private tuple::less< tuple::tag<T> >
{
};

int main()
{
  pair<int> value;
  (void)value;
  return 0;
}
