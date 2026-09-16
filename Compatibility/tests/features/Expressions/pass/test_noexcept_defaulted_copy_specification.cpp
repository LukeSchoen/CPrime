struct Value {
    Value() noexcept {}
    Value(const Value&) noexcept {}
};
struct SafeCopy {
    Value value;
    SafeCopy() = default;
    SafeCopy(const SafeCopy&) = default;
};
struct ThrowingCopy {
    Value value;
    ThrowingCopy() = default;
    ThrowingCopy(const ThrowingCopy&) noexcept(false) = default;
};
struct RiskyValue {
    RiskyValue() noexcept {}
    RiskyValue(const RiskyValue&) {}
};
struct ForcedSafeCopy {
    RiskyValue value;
    ForcedSafeCopy() = default;
    ForcedSafeCopy(const ForcedSafeCopy&) noexcept = default;
};
int main() {
    SafeCopy safe;
    ThrowingCopy throwing;
    ForcedSafeCopy forced;
    static_assert(noexcept(SafeCopy(safe)), "inferred copy specification");
    static_assert(!noexcept(ThrowingCopy(throwing)), "explicit copy specification");
    static_assert(noexcept(ForcedSafeCopy(forced)), "explicit nonthrowing boundary");
    return 0;
}
