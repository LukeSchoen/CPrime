// EXPECT_EXIT: 0
struct Owner {
    struct Item { int value; };
    Item items[2];
    int sum();
    static int count();
};
int Owner::sum() {
    int result = 0;
    for (int i = 0; i < 2; ++i) {
        Item &item = items[i];
        result += item.value;
    }
    { const Item &item = items[0]; result += item.value; }
    return result;
}
int Owner::count() {
    using Alias = Item;
    Alias value = Item{7};
    return value.value;
}
int main() {
    Owner owner;
    owner.items[0].value = 2;
    owner.items[1].value = 3;
    return owner.sum() == 7 && Owner::count() == 7 ? 0 : 1;
}
