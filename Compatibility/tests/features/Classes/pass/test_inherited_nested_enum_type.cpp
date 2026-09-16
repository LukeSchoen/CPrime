// EXPECT_EXIT: 0
struct First { enum Kind { low = 3, high = 7 }; struct Nested { int value; }; };
struct Second { enum Other { extra = 11 }; };
struct Derived : First, Second {};
int main() {
    Derived::Kind kind = Derived::high;
    Derived::Other other = Derived::extra;
    Derived::Nested nested = {13};
    return kind != 7 || other != 11 || nested.value != 13;
}
