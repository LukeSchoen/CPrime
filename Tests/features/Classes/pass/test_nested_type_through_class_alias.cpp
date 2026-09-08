// EXPECT_EXIT: 0
struct Outer { struct Inner { struct Leaf { int value; Leaf() : value(13) {} }; }; };
struct Aliases { typedef Outer Q; };
struct Derived : Aliases::Q::Inner::Leaf {};
int main() { Aliases::Q::Inner::Leaf object; Derived derived; return object.value != 13 || derived.value != 13; }
