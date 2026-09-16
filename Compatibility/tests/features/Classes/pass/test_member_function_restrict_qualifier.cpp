// EXPECT_EXIT: 0
// GNU restrict on a member function declarator is accepted and ignored.
struct A
{
    int f() __restrict__ { return 3; }
    int g() __restrict__;
};

int A::g() __restrict__ { return 4; }

int main()
{
    A object;
    return object.f() != 3 || object.g() != 4;
}
