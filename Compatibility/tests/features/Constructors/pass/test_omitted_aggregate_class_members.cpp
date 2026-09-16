struct Item {
    int value;
    Item() : value(42) {}
    Item(int value) : value(value) {}
};
struct Aggregate { Item first, second, third; };
Aggregate global = { 1 };
int main() {
    Aggregate local = { 2 };
    return global.first.value != 1 || global.second.value != 42 || global.third.value != 42
        || local.first.value != 2 || local.second.value != 42 || local.third.value != 42;
}
