struct Owner {
    enum Kind { First, Second };
    struct Item { int value; };
    int read(Kind kind, const Item *item);
};
int Owner::read(Kind kind, const Item *item) {
    return item->value + (kind == Second ? 2 : 1);
}
int main() {
    Owner owner;
    Owner::Item item;
    item.value = 40;
    return owner.read(Owner::Second, &item) != 42;
}
