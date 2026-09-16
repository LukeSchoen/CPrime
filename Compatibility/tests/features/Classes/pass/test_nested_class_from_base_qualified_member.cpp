// EXPECT_EXIT: 0
// A qualified member access may name a nested class inherited from a base of
// the qualifier's class: `x2.A::Linner::ii_inner` where Linner comes from L, a
// base of A.
struct L
{
    int ii;
    struct Linner
    {
        int ii_inner;
        void foo_inner(int b) { ++b; }
    };
};

class A : public L {};
class B : public L {};
class C : public A, public B {};

int main()
{
    C::A::Linner x2;
    x2.A::Linner::ii_inner = 6;
    x2.A::Linner::foo_inner(x2.A::Linner::ii_inner);
    return x2.A::Linner::ii_inner != 6;
}
