// EXPECT_EXIT: 0
// A qualified member call may name a base class through another base of the
// object's class (`D::A::foo`) and must still bind to the shared virtual base
// subobject.
int calls = 0;
struct A { virtual void foo(int a) { calls += a; } };
struct B { virtual void bar(int) {} };
struct D : public virtual A, public virtual B {};
struct C : private D, public virtual A, public virtual B
{
    virtual void foo(int a) { A::foo(a); D::A::foo(a); }
};

int main()
{
    C c;
    c.foo(2);
    return calls != 4;
}
