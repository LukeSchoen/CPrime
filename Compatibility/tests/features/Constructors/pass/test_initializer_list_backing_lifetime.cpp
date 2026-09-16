// EXPECT_EXIT: 0
#include <initializer_list>
int live, destroyed;
struct Item {
    int value;
    Item(int n) : value(n) { ++live; }
    Item(const Item& x) : value(x.value) { ++live; }
    ~Item() { --live; destroyed = destroyed * 10 + value; }
};
int consume(std::initializer_list<Item> values) {
    return live == 2 && values.begin()[0].value == 4 && values.begin()[1].value == 5;
}
struct ImplicitChoice {
    int value;
    explicit ImplicitChoice(int n) : value(-n) {}
    ImplicitChoice(double n) : value(int(n)) {}
};
int both(std::initializer_list<int> a, std::initializer_list<int> b) {
    return a.begin()[0] == 1 && a.begin()[3] == 4
        && b.begin()[0] == 5 && b.begin()[3] == 8;
}
struct Aggregate {
    int value;
    ~Aggregate() { destroyed = destroyed * 10 + value; }
};
struct Pair {
    int first, second;
    Pair(int a, int b) : first(a), second(b) {}
};
int main() {
    {
        std::initializer_list<Item> values = {1, 2, 3};
        if (live != 3 || destroyed) return 1;
        std::initializer_list<Item> copy = values;
        if (copy.begin() != values.begin() || live != 3) return 2;
        if (values.begin()[2].value != 3) return 3;
    }
    if (live || destroyed != 321) return 4;
    destroyed = 0;
    if (!consume({4, 5})) return 5;
    if (live || destroyed != 54) return 6;
    std::initializer_list<ImplicitChoice> choices = {7, 8};
    if (choices.begin()[0].value != 7 || choices.begin()[1].value != 8) return 7;
    if (!both({1, 2, 3, 4}, {5, 6, 7, 8})) return 8;
    destroyed = 0;
    {
        std::initializer_list<Aggregate> aggregates = {{6}, {7}};
        if (destroyed || aggregates.begin()[0].value != 6) return 9;
    }
    if (destroyed != 76) return 10;
    std::initializer_list<Pair> pairs = {{1, 2}, {3, 4}};
    return pairs.begin()[0].second != 2 || pairs.begin()[1].first != 3;
}
