// EXPECT_COMPILE_FAIL: 1
struct Base { int value; };
struct Virtual : virtual Base {};
struct Ordinary : Base {};
struct Derived : Virtual, Ordinary {};
int read(Derived &object, int Base::*member) { return object.*member; }
int main() { return 0; }
