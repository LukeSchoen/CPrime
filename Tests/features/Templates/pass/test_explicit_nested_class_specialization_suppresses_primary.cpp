// EXPECT_EXIT: 0
// An explicit specialization of a non-template nested class replaces only
// that nested declaration.  The primary remains available for a different
// outer specialization, whose dependent base and using-declaration must
// still be instantiated.
template <class T> struct outer
{
    struct member : T
    {
        using T::value;
        member() { value = 7; }
    };
};

struct base { int value; };

template <> struct outer<int>::member
{
    member() {}
};

int main()
{
    outer<base>::member dependent;
    outer<int>::member specialized;
    (void)specialized;
    return dependent.value != 7;
}
