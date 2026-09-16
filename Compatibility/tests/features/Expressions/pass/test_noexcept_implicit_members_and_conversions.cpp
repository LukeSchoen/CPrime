struct Safe {
    Safe() noexcept {}
    Safe(const Safe&) noexcept {}
    Safe& operator=(const Safe&) noexcept { return *this; }
    operator int() const noexcept { return 1; }
    int operator+(int) const noexcept { return 2; }
};
struct Risky {
    Risky() {}
    operator int() const { return 3; }
    int operator+(int) const { return 4; }
};
struct SafeOwner { Safe value; };
struct RiskyOwner { Risky value; };
int initializer_calls;
int safe_value() noexcept { ++initializer_calls; return 1; }
int risky_value() { ++initializer_calls; return 2; }
struct SafeInitializer { int value = safe_value(); };
struct RiskyInitializer { int value = risky_value(); };
struct SafeDefaulted { Safe value; SafeDefaulted() = default; };
struct ExplicitThrowing { Safe value; ExplicitThrowing() noexcept(false) = default; };
struct DependentInitializer { int first = safe_value(); int second = first + 1; };
int consume(int) noexcept { return 0; }
int main() {
    Safe safe;
    Risky risky;
    SafeOwner owner;
    static_assert(noexcept(SafeOwner()), "implicit safe default constructor");
    static_assert(!noexcept(RiskyOwner()), "implicit throwing default constructor");
    static_assert(noexcept(SafeOwner(owner)), "implicit safe copy constructor");
    static_assert(noexcept(owner = owner), "implicit safe copy assignment");
    static_assert(noexcept(consume(safe)), "safe conversion");
    static_assert(!noexcept(consume(risky)), "throwing conversion");
    static_assert(noexcept(safe + 1), "safe overloaded operator");
    static_assert(!noexcept(risky + 1), "throwing overloaded operator");
    static_assert(noexcept(SafeInitializer()), "safe member initializer");
    static_assert(!noexcept(RiskyInitializer()), "throwing member initializer");
    static_assert(noexcept(SafeDefaulted()), "defaulted constructor");
    static_assert(!noexcept(ExplicitThrowing()), "explicit specification wins");
    static_assert(noexcept(DependentInitializer()), "member initializer scope");
    if (initializer_calls != 0) return 1;
    if (SafeInitializer().value != 1 || initializer_calls != 1) return 2;
    if (RiskyInitializer().value != 2 || initializer_calls != 2) return 3;
    if (DependentInitializer().second != 2 || initializer_calls != 3) return 4;
    return 0;
}
