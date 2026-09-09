// EXPECT_EXIT: 0
struct Base {
    operator const char*() const { return "base"; }
    operator int() const { return 7; }
};
struct Derived : Base { operator const char*() const { return "derived"; } };
struct Leaf : Derived {};
int main() {
    Leaf object;
    const char* text = object;
    int number = object;
    return text[0] != 'd' || number != 7;
}
