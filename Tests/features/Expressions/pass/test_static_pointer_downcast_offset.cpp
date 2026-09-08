// EXPECT_EXIT: 0
struct Prefix { int first; };
struct Base { int second; };
struct Derived : Prefix, Base { int third; };
Derived* recover(Base* value) { return static_cast<Derived*>(value); }
int main() {
    Derived value;
    value.first = 11; value.second = 23; value.third = 31;
    Base* base = &value;
    Derived* result = recover(base);
    return result != &value || result->third != 31 || recover(0) != 0;
}
