// EXPECT_EXIT: 0
struct Base { struct Item { int value; }; };
struct Derived : Base {
    void Item();
    struct Item stored;
    int read();
};
int Derived::read() {
    struct Item local = {7};
    return local.value + stored.value;
}
namespace Space {
struct Entry { int value; };
struct Owner { int read(); };
int Owner::read() { struct Entry local = {11}; return local.value; }
}
int main() {
    Derived object; object.stored.value = 3;
    Space::Owner owner;
    return object.read() != 10 || owner.read() != 11;
}
