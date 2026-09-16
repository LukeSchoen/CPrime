// EXPECT_EXIT: 0
// An inline or constexpr specifier may precede 'friend' in an in-class friend
// function definition (`inline friend float f() { ... }`).  The lifecycle and
// constructor probes restore such specifiers for ordinary members, so the
// member parser used to read the specifier as the member type and reject the
// declaration with "invalid type for 'friend'".
template<int> class Program { };

template<> class Program<0> {
public:
    inline friend float EvalNextArg() { return 1.0f; }
};

struct Owner {
    constexpr friend int CVal() { return 5; }
    inline friend int Gimme(int v) { return v + 1; }
};

int main() { return 0; }
