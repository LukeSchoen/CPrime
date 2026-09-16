// EXPECT_COMPILE_FAIL: 1
struct Base { int value; };
struct Left : Base {};
struct Right : Base {};
struct Derived : virtual Left, virtual Right {};
int read(Derived &object, int Base::*member) { return object.*member; }
int main() { return 0; }
