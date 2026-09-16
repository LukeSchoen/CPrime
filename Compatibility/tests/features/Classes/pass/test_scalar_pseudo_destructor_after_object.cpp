// EXPECT_EXIT: 0
// Scalar operands may be pseudo-destroyed through a qualified type name after
// `.` or `->`, including through a dependent template parameter.
template <class T> class A
{
    T q;
public:
    ~A()
    {
        q.T::~T();
        q.~T();
        (&q)->T::~T();
        (&q)->~T();
    }
};

typedef char *cp;
typedef int I;

int main()
{
    A<int> a;
    A<cp> b;
    int i = 0;
    cp c = 0;
    i.~I();
    i.I::~I();
    (&i)->~I();
    (&i)->I::~I();
    c.~cp();
    c.cp::~cp();
    (&c)->~cp();
    (&c)->cp::~cp();
    return i;
}
