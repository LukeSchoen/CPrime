// EXPECT_EXIT: 0
int value = 99;
struct Base { protected: static int value; };
int Base::value = 3;
struct Owner : private Base {
    struct Nested { int read(); void write(int); };
    struct Own { int value; int read() { return value; } };
};
int Owner::Nested::read() { return value; }
void Owner::Nested::write(int next) { value = next; }
int main() {
    Owner::Nested object;
    if (object.read() != 3) return 1;
    object.write(7);
    Owner::Own own;
    own.value = 11;
    return object.read() != 7 || own.read() != 11 || value != 99;
}
