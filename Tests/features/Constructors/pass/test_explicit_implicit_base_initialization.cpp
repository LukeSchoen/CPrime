// EXPECT_EXIT: 0
struct Plain { int value; };
struct Empty {};
struct Derived : Plain, Empty {
    Derived() : Plain(), Empty(Empty()) {}
    Derived(Plain p) : Plain(p), Empty() {}
};
template<class T> struct Wrapper : T { Wrapper() : T() {} };
int main() {
    Derived zero;
    Plain source = {19};
    Derived copied(source);
    Wrapper<Plain> generic;
    if (zero.value != 0) return 1;
    if (copied.value != 19) return 2;
    if (generic.value != 0) return 3;
    return 0;
}
