// EXPECT_EXIT: 0
// A member typedef may qualify a pseudo-destructor or qualified member of the
// object's own class: `px->classtype::~classtype()` and `px->classtype::value`.
struct X
{
    typedef X classtype;
    int value;
};

int main()
{
    X x;
    X *px = &x;
    px->value = 4;
    px->classtype::value = 5;
    px->classtype::~classtype();
    return px->value != 5;
}
