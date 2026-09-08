// EXPECT_COMPILE_FAIL: 1
struct Item { int x, y; };
struct Sink {
    void add(const Item &, Item &&) {}
    void add(Item &&, const Item &) {}
};
int main() { Sink sink; sink.add({1, 2}, {3, 4}); }
