// EXPECT_EXIT: 0
struct Base {
    virtual operator int() const { return 7; }
    explicit operator char*() const { return 0; }
};
struct Derived : Base { operator int() const { return 19; } };
int main() {
    Derived d;
    Base &b = d;
    if ((int)b != 19 || static_cast<int>(b) != 19) return 1;
    if ((char*)b != 0 || static_cast<char*>(b) != 0) return 2;
    return 0;
}
