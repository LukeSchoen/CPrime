// EXPECT_EXIT: 0
// A friend declaration inside a member class may name the enclosing class
// template without an argument list, including while the out-of-class member
// body is replayed by an explicit instantiation.
template<class C> struct traits
{
};

template<class C, class Tr = traits<C> > class outer;

template<class C, class Tr> class outer
{
public:
    class inner;
};

template<class C, class Tr> class outer<C, Tr>::inner
{
    friend class outer;
};

template class outer<char, traits<char> >::inner;

int main()
{
    return 0;
}
