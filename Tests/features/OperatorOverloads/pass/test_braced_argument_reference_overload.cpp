struct Item { int first, second; };
struct Sink {
    int add(const Item &) { return 1; }
    int add(Item &&item) { return item.first + item.second; }
};
int choose(const Item &) { return 1; }
int choose(Item &&item) { return item.first + item.second; }
const Item *address(const Item &item) { return &item; }
const Item *address(Item &&) { return 0; }
int main() {
    Sink sink;
    if (sink.add({2, 3}) != 5) return 1;
    if (choose({3, 4}) != 7) return 2;
    Item item = {4, 5};
    if (sink.add(item) != 1 || choose(item) != 1) return 3;
    if (sink.add({item}) != 1 || choose({item}) != 1) return 4;
    if (address({item}) != &item) return 5;
    if (sink.add({}) != 0 || choose({}) != 0) return 6;
    return 0;
}
