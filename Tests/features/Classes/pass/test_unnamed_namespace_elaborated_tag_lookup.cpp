// EXPECT_EXIT: 0
// An unnamed namespace is implicitly visible in its enclosing namespace. An
// elaborated tag used from a global function must find that record rather than
// declaring a fresh block-local class. A forward declaration following it must
// still declare the ordinary global tag.
namespace {
struct Hidden {
    int value;
};
}

struct Global;
struct Global {
    int value;
};

int main()
{
    struct Hidden hidden = { 7 };
    struct Hidden *hidden_pointer = &hidden;
    struct Global global = { 9 };
    return hidden_pointer->value != 7 || global.value != 9;
}
