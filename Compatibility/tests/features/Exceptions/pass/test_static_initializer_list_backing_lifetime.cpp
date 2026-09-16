// EXPECT_EXIT: 0
#include <initializer_list>
#include <stdlib.h>
int live, destroyed;
bool fail_eight = true;
struct Item {
    int value;
    Item(int n) : value(n) { if (n == 8 && fail_eight) throw n; ++live; }
    ~Item() { --live; destroyed = destroyed * 10 + value; }
};
struct ExitCheck {
    ~ExitCheck() { if (live || destroyed != 78721) abort(); }
} exit_check;
std::initializer_list<Item> global_values = {1, 2};
std::initializer_list<int> global_numbers = {3, 4};
std::initializer_list<Item> global_empty = {};
const std::initializer_list<Item>& local_values() {
    static std::initializer_list<Item> values = {7, 8};
    return values;
}
int main() {
    if (live != 2 || global_values.size() != 2 || global_values.begin()[1].value != 2
        || global_numbers.begin()[1] != 4 || global_empty.size() != 0) return 1;
    try { local_values(); return 2; }
    catch (int value) { if (value != 8 || live != 2 || destroyed != 7) return 3; }
    fail_eight = false;
    const Item* first = local_values().begin();
    if (live != 4 || first[0].value != 7 || first[1].value != 8) return 4;
    return local_values().begin() != first || live != 4;
}
