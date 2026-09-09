// EXPECT_EXIT: 0
struct Base { int value; int pointer; int inherited; };
struct Derived : Base {
    union { double value; void* pointer; };
};
int main() {
    Derived object;
    object.value = 1.5;
    object.inherited = 7;
    if (object.value != 1.5 || object.inherited != 7) return 1;
    object.pointer = &object;
    object.Base::pointer = 19;
    return object.pointer != &object || object.Base::pointer != 19;
}
