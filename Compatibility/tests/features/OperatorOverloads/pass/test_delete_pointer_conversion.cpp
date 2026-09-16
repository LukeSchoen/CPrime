// EXPECT_EXIT: 0
int destroyed;
struct Item { ~Item() { ++destroyed; } };
struct Pointer {
    Item *value;
    operator Item*() const { return value; }
    operator bool() { return false; }
};
struct Derived : Pointer {};
int main() {
    Pointer p;
    p.value = new Item;
    delete p;
    Derived d;
    d.value = new Item;
    delete d;
    p.value = new Item[3];
    delete[] p;
    return destroyed != 5;
}
