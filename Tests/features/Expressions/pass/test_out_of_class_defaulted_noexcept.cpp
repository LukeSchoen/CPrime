static int defaults, copies, assignments, destructions;
struct Value {
    int number;
    Value() noexcept : number(17) { ++defaults; }
    Value(const Value& other) noexcept : number(other.number) { ++copies; }
    Value& operator=(const Value& other) noexcept { ++assignments; number = other.number; return *this; }
    ~Value() { ++destructions; }
};
struct Owner {
    Value value;
    Owner();
    Owner(const Owner&);
    Owner& operator=(const Owner&);
    ~Owner();
};
Owner::Owner() = default;
Owner::Owner(const Owner&) = default;
Owner& Owner::operator=(const Owner&) = default;
Owner::~Owner() = default;
struct SafeOwner {
    Value value;
    SafeOwner() noexcept;
    SafeOwner(const SafeOwner&) noexcept;
    SafeOwner& operator=(const SafeOwner&) noexcept;
};
SafeOwner::SafeOwner() noexcept = default;
SafeOwner::SafeOwner(const SafeOwner&) noexcept = default;
SafeOwner& SafeOwner::operator=(const SafeOwner&) noexcept = default;
int main() {
    {
    Owner first, second;
    static_assert(!noexcept(Owner()), "previous default-constructor declaration");
    static_assert(!noexcept(Owner(first)), "previous copy-constructor declaration");
    static_assert(!noexcept(first = second), "previous assignment declaration");
    if (defaults != 2 || copies || assignments || destructions) return 1;
    Owner copied(first);
    if (copies != 1 || copied.value.number != 17) return 2;
    second.value.number = 29;
    if (&(first = second) != &first || assignments != 1 || first.value.number != 29) return 3;
    }
    if (destructions != 3) return 4;
    {
        SafeOwner source;
        static_assert(noexcept(SafeOwner()), "matching default declaration");
        static_assert(noexcept(SafeOwner(source)), "matching copy declaration");
        SafeOwner destination(source);
        static_assert(noexcept(destination = source), "matching assignment declaration");
        if (&(destination = source) != &destination || destination.value.number != 17) return 5;
    }
    return defaults != 3 || copies != 2 || assignments != 2 || destructions != 5;
}
