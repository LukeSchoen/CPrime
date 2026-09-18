// EXPECT_EXIT: 0

// A class-scope class template is registered from the class that contains it
// and parsed again when it is instantiated.  Its saved declaration keeps the
// relative qualifier `mpl::if_c` and the enclosing class's own typedefs, so
// the replay has to run in the containing class's namespace.  This is the
// reduced shape of boost/log/attributes/attribute_set.hpp's nested iterator.

namespace mpl {

template <bool condition, class then_type, class else_type>
struct if_c
{
    typedef then_type type;
};

template <class then_type, class else_type>
struct if_c<false, then_type, else_type>
{
    typedef else_type type;
};

} // namespace mpl

namespace attributes {

class attribute
{
public:
    int value;
};

class attribute_set
{
public:
    typedef attribute mapped_type;
    typedef attribute& reference;
    typedef attribute const& const_reference;

    template <bool is_const>
    class iter
    {
    public:
        typedef typename mpl::if_c<
            is_const,
            attribute_set::const_reference,
            attribute_set::reference
        >::type reference_type;
    };
};

} // namespace attributes

int main()
{
    attributes::attribute a;
    a.value = 7;

    attributes::attribute_set::iter<false>::reference_type r = a;
    attributes::attribute_set::iter<true>::reference_type c = a;

    r.value = 9;
    if (&r != &a || &c != &a)
        return 1;
    return c.value == 9 ? 0 : 1;
}
