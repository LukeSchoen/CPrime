// EXPECT_EXIT: 0
struct Prefix { int first; };
struct Base { int second; };
struct Derived : Prefix, private Base {
    int third;
    Base* base() { return this; }
};
Derived* recover(const Base* value) { return (Derived*)value; }
Derived& recover_reference(const Base& value) { return (Derived&)value; }
int main() {
    Derived value;
    value.third = 31;
    const Base* base = value.base();
    return recover(base) != &value || recover(base)->third != 31
        || &recover_reference(*base) != &value || recover(0) != 0;
}
