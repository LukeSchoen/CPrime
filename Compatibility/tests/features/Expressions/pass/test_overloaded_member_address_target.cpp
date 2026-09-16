// EXPECT_EXIT: 0
struct Object {
    int get() { return 3; }
    int get() const { return 7; }
    int get(int n) { return n + 11; }
};
int (Object::*global)() const = &Object::get;
int main() {
    Object object;
    int (Object::*plain)() = &Object::get;
    int (Object::*argument)(int) = &Object::get;
    plain = &Object::get;
    return (object.*plain)() != 3 || (object.*global)() != 7
        || (object.*argument)(2) != 13;
}
