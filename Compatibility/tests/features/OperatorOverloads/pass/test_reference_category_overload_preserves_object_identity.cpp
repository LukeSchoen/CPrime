// EXPECT_EXIT: 0
struct Item { int value; };
int choose(Item& item) { item.value += 1; return 1; }
int choose(const Item& item) { return item.value == 7 ? 2 : 9; }
int choose(Item&& item) { return item.value == 11 ? 3 : 9; }
int main() {
    Item mutable_item = {3}; const Item constant_item = {7};
    return choose(mutable_item) != 1 || mutable_item.value != 4
        || choose(constant_item) != 2 || choose(Item{11}) != 3;
}
