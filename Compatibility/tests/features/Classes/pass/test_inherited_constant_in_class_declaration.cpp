// EXPECT_EXIT: 0
enum { Width = 9 };
struct Base { enum { Width = 3 }; static const int Height = 2; };
struct Derived : Base { char data[Width][Height]; };
int main() {
    enum { Width = 11 };
    struct Local : Base { char data[Width]; };
    Derived derived;
    Local local;
    if (sizeof(derived.data) != 6) return 1;
    if (sizeof(local.data) != 3) return 2;
    return Width != 11 ? 3 : 0;
}
