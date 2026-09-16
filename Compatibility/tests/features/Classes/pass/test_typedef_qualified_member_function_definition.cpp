// EXPECT_EXIT: 0
// An out-of-class member function definition may qualify the member with a
// typedef of its class.
struct A
{
    int f();
    int g();
};

typedef A Alias;
int Alias::f() { return 3; }
int Alias::g() { return 4; }

int main()
{
    A object;
    return object.f() != 3 || object.g() != 4;
}
