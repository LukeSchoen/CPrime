// EXPECT_EXIT: 0
namespace Left {
struct Base { int value; };
struct Derived : Base { template<class T> static int convert(T v) { return (int)v + 1; } };
}
namespace Right {
struct Base { int other; };
struct Derived : Base { template<class T> static int convert(T v) { return (int)v + 2; } };
}
struct Unrelated : Right::Derived {};
struct Multiple : Left::Derived, Unrelated {};
int main() {
    Multiple x;
    Left::Base *a = &x;
    Right::Base *b = &x;
    a->value = 17;
    b->other = 29;
    if (x.value != 17 || x.other != 29) return 1;
    if (Left::Derived::convert(3) != 4) return 2;
    if (Right::Derived::convert(3) != 5) return 3;
    return 0;
}
