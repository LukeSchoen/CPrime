// EXPECT_EXIT: 0
struct Base {
    int f(int) const { return 1; }
    int f(double) const { return 2; }
    int g() const;
protected:
    int value;
};
struct Derived : Base {
    using Base::f;
    using Base::g;
    using Base::value;
    int g() const { return 3; }
};
int main() {
    Derived d;
    d.value = 7;
    return d.f(1) != 1 || d.f(1.0) != 2 || d.g() != 3 || d.value != 7;
}
