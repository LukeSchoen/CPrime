// EXPECT_EXIT: 0
struct First {
    First() {}
    First(const First&) {}
    virtual int first() const { return 1; }
};
struct Second {
    Second() {}
    Second(Second&) {}
    virtual int second() const { return 2; }
};
struct Derived : First, Second {
    int first() const { return 11; }
    int second() const { return 22; }
};
Derived make() { Derived value; return static_cast<Derived&>(value); }
struct Holder { int prefix; Derived members[2]; };
int inspect(Derived value) {
    First& first = value;
    Second& second = value;
    return first.first() == 11 && second.second() == 22;
}
int main() {
    Derived original;
    Derived copy = original;
    Holder holder;
    Holder nested = holder;
    return !inspect(original) || !inspect(copy) || !inspect(make())
        || !inspect(nested.members[0]) || !inspect(nested.members[1]);
}
