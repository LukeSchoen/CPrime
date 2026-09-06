// EXPECT_EXIT: 0
#include <initializer_list>
int live, destroyed, entered;
struct Item {
    int value;
    Item(int n) : value(n) { if (n == 3) throw n; ++live; }
    Item(const Item& x) : value(x.value) { ++live; }
    ~Item() { --live; destroyed = destroyed * 10 + value; }
};
void consume(std::initializer_list<Item>) { ++entered; }
int main() {
    try { std::initializer_list<Item> values = {1, 2, 3, 4}; return 1; }
    catch (int value) { if (value != 3 || live || destroyed != 21) return 2; }
    destroyed = 0;
    try { consume({1, 2, 3, 4}); return 3; }
    catch (int value) { if (value != 3 || live || destroyed != 21 || entered) return 4; }
    return 0;
}
