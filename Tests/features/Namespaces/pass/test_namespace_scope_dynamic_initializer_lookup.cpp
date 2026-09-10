// EXPECT_EXIT: 0
//
// A namespace-scope object with a non-constant initializer is initialized by a
// synthesized function that runs before main.  That function must replay the
// initializer inside the namespace the object was declared in, so unqualified
// names in the initializer still find members declared earlier in the same
// namespace (objects, functions and class-construction members).

namespace Outer {
    int base = 5;
    int add(int value) { return value + 1; }

    int scaled = base * 2;
    int called = add(scaled);
    int combined = called + base;

    struct Counter { int v; Counter() : v(4) {} };
    Counter counter;
    int from_ctor = counter.v + base;

    namespace Inner {
        int seed = 3;
        int grown = Outer::base + seed;
    }
}

namespace Later {
    int first = 6;
}

namespace Later {
    int second = first * 2;
    int third = Later::first + second;
}

int main()
{
    if (Outer::scaled != 10 || Outer::called != 11 || Outer::combined != 16)
        return 1;
    if (Outer::counter.v != 4 || Outer::from_ctor != 9)
        return 2;
    if (Outer::Inner::grown != 8)
        return 3;
    if (Later::second != 12 || Later::third != 18)
        return 4;
    return 0;
}
