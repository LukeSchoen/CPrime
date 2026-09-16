// EXPECT_EXIT: 0
int copies;
struct Padding { int padding; };
struct Base {
    int value;
    Base(int v = 0) : value(v) {}
    Base(const Base& other) : value(other.value) { ++copies; }
};
struct Derived : Padding, Base { Derived() : Base(7) {} };
Base& select(bool condition, Base& base, Derived& derived) {
    return condition ? base : derived;
}
int main() {
    Base base(3); Derived derived;
    if (&select(false, base, derived) != static_cast<Base*>(&derived)
        || &select(true, base, derived) != &base || copies) return 1;
    select(false, base, derived).value = 12;
    return derived.value != 12 || base.value != 3 || copies;
}
