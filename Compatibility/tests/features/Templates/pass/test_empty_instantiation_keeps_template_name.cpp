/* An instance of a variadic class template asked for with no arguments used to
   be named exactly like its template, so the instance's class record was
   registered under the template's own token.  A later qualified spelling of
   the template then resolved to that `list<>` instance instead of the
   template, leaving its argument list unparsed.  boost::mp11 hits this in
   every header that instantiates `mp_list<>` inside an explicit
   specialization (`mp_set_union_impl<>` in boost/mp11/set.hpp), after which
   boost/parameter/aux_/set.hpp's own `::boost::mp11::mp_list<>` set0 failed
   with "identifier expected".  The namespace is named without a leading `::`
   so the case also compiles inside a unity group. */
namespace ns
{

template<class... T>
struct list
{
    char storage[sizeof...(T) + 1];
};

} // namespace ns

template<class... L>
struct holder
{
};

template<>
struct holder<>
{
    using type = ns::list<>;
};

int main()
{
    /* The template name must still name the template here, so the two
       argument lists stay distinct types. */
    return sizeof(ns::list<>) == 1 && sizeof(ns::list<int, char>) == 3 ? 0 : 1;
}
