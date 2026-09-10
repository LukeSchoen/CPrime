// EXPECT_EXIT: 0
// A non-static data member may be named in an unevaluated operand inside the
// initializer of a static data member even though no object exists yet.  The
// identifier lookup used to require a `this` receiver, so `sizeof (member)`
// was rejected as undeclared in the class that declares the member.
struct A {
    int b[4];
    static const int count = sizeof(b) / sizeof(b[0]);
};
int counts[A::count == 4 ? 1 : -1];

struct P {
    struct B { int value; } *b;
    static const int pointer_size = sizeof(b);
};
int pointers[P::pointer_size == sizeof(int*) ? 1 : -1];

template<class>
struct T {
    struct B {} *b;
    static const int c = sizeof(b) / sizeof(b[0]);
};
int template_values[T<int>::c == sizeof(int*) ? 1 : -1];

int main() {
    return 0;
}
