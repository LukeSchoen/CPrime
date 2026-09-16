// EXPECT_EXIT: 0
// The static receiver spelling of an unnamed-namespace virtual interface is
// used through an elaborated tag in a global function. Keep the covariant
// return adjustment and the implicit unnamed-namespace lookup together.
namespace {
struct Prefix {
    char padding[8];
    virtual Prefix *self() { return this; }
};

struct Interface {
    int value;
    explicit Interface(int initial) : value(initial) {}
    virtual Interface *next(int) = 0;
};

struct Implementation : Prefix, Interface {
    explicit Implementation(int initial) : Interface(initial) {}
    Implementation *next(int) { return &result; }
    static Implementation result;
};

Implementation Implementation::result(11);
}

int main()
{
    Implementation object(2);
    struct Interface *base = &object;
    struct Interface *result = base->next(4);
    return result->value != 11;
}
