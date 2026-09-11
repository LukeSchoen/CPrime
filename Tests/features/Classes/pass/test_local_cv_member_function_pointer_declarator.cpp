// EXPECT_EXIT: 0
// A block-scope pointer-to-member-function declarator may carry the
// cv-qualifiers of the member function it will point to.  The trailing
// qualifier must not make the statement look like an expression.
class A
{
public:
    int f() { return 0; }
    int f() const { return 1; }
};

class B : public A
{
};

int main()
{
    int (B::*plain)() = &B::f;
    int (B::*qualified)() const = &B::f;
    B object;
    return (object.*plain)() != 0 || (object.*qualified)() != 1;
}
